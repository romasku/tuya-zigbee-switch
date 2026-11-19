#include "zigbee/tuya_secondary_mcu.h"
#include "hal/printf_selector.h"
#include "hal/tasks.h"
#include "hal/timer.h"
#include "hal/uart.h"

#include <string.h>

/* ---- framing -------------------------------------------------------------- */

#define FRAME_HEADER_LEN    8 /* 55 AA 02 seq seq cmd len len */
#define FRAME_VERSION       0x02

static uint8_t           g_enabled           = 0;
static hal_uart_config_t g_uart_cfg          = { 0 };
static uint16_t          g_tx_seq            = 0;
static uint8_t           g_report_is_passive = 0;

static uint16_t next_tx_seq(void) {
    uint16_t seq = g_tx_seq;

    g_tx_seq = (g_tx_seq >= 0xFFF0) ? 0 : (uint16_t)(g_tx_seq + 1);
    return seq;
}

static uint8_t checksum(const uint8_t *buf, uint16_t len) {
    uint8_t sum = 0;

    for (uint16_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}

/* A write the UART will not take means the link is wedged (pinmux reassigned,
 * clock moved, odd state after a re-init). Silence is not evidence of that:
 * the MCU is quiet by design. Repeated write failures are, so they trigger a
 * re-init. */
#define UART_FAILURES_BEFORE_REINIT    3
static uint8_t g_write_failures = 0;

static int uart_send(const uint8_t *buf, uint16_t len) {
    if (hal_uart_write(buf, len, NULL) == HAL_UART_OK) {
        g_write_failures = 0;
        return 0;
    }
    if (++g_write_failures >= UART_FAILURES_BEFORE_REINIT) {
        printf("Secondary MCU link stuck, reinitialising UART\r\n");
        g_write_failures = 0;
        hal_uart_deinit();
        hal_uart_init(&g_uart_cfg);
    }
    return -1;
}

/* Builds and sends one frame. For a DP write `data` is the DP value and the
 * DP unit header is added here. */
static int send_frame(uint8_t cmd, uint16_t seq, uint8_t dpid, uint8_t dp_type,
                      const uint8_t *data, uint16_t data_len) {
    uint8_t  buf[FRAME_HEADER_LEN + 4 + 32 + 1];
    uint16_t body = (uint16_t)(cmd == TUYA_MCU_CMD_WRITE ? data_len + 4 : data_len);
    uint16_t idx  = 0;

    if (data_len > 32 || (data_len > 0 && data == NULL)) {
        return -1;
    }
    buf[idx++] = 0x55;
    buf[idx++] = 0xAA;
    buf[idx++] = FRAME_VERSION;
    buf[idx++] = (uint8_t)(seq >> 8);
    buf[idx++] = (uint8_t)seq;
    buf[idx++] = cmd;
    buf[idx++] = (uint8_t)(body >> 8);
    buf[idx++] = (uint8_t)body;
    if (cmd == TUYA_MCU_CMD_WRITE) {
        buf[idx++] = dpid;
        buf[idx++] = dp_type;
        buf[idx++] = (uint8_t)(data_len >> 8);
        buf[idx++] = (uint8_t)data_len;
    }
    if (data_len > 0) {
        memcpy(&buf[idx], data, data_len);
        idx = (uint16_t)(idx + data_len);
    }
    buf[idx] = checksum(buf, idx);
    idx++;

    int ok = uart_send(buf, idx) == 0;
    if (!ok) {
        return -1;
    }
    return 0;
}

int tuya_secondary_mcu_send_reply(uint8_t cmd, uint16_t seq,
                                  const uint8_t *payload, uint16_t len) {
    if (!g_enabled) {
        return -1;
    }
    return send_frame(cmd, seq, 0, 0, payload, len);
}

/* ---- TX queue: one frame in flight --------------------------------------
 *
 * The MCU answers every DP write with a report of that DP. Writing the next
 * frame before that answer arrives made this MCU family unreliable (OpenBeken
 * needed its "TuyaMCU queue" flag for it), so frames we start go out one at a
 * time: the next one when the MCU has answered, or after TX_ACK_TIMEOUT_MS
 * (a brightness write to a lit channel is answered only after the MCU's own
 * fade, ~1 s; the timeout lets the next write through meanwhile).
 */
#define TX_QUEUE_LEN         8
#define TX_ACK_TIMEOUT_MS    150

typedef struct {
    uint8_t cmd;     /* TUYA_MCU_CMD_WRITE or a command we start */
    uint8_t dpid;    /* DP writes only */
    uint8_t dp_type;
    uint8_t len;
    uint8_t value[16];
} tx_item_t;

static tx_item_t  g_txq[TX_QUEUE_LEN];
static uint8_t    g_txq_head  = 0;
static uint8_t    g_txq_count = 0;
static uint8_t    g_tx_busy   = 0;
static tx_item_t  g_tx_in_flight;
static hal_task_t g_tx_timer;
static tuya_secondary_mcu_tx_callback_t g_tx_callback = NULL;

static uint8_t tx_item_id(const tx_item_t *item) {
    return item->cmd == TUYA_MCU_CMD_WRITE ? item->dpid : item->cmd;
}

static void tx_kick(void);

static void tx_arm_timer(void) {
    hal_tasks_unschedule(&g_tx_timer);
    hal_tasks_schedule(&g_tx_timer, TX_ACK_TIMEOUT_MS);
}

/* Ack timeout of the frame in flight, or the retry after a failed write. */
static void tx_timer_fired(void *arg) {
    (void)arg;
    if (g_tx_busy) {
        g_tx_busy = 0;
    }
    tx_kick();
}

static int tx_send_now(const tx_item_t *item) {
    uint16_t seq = next_tx_seq();

    if (send_frame(item->cmd, seq, item->dpid, item->dp_type, item->value, item->len) != 0) {
        return -1;
    }
    g_tx_in_flight = *item;
    g_tx_busy      = 1;
    tx_arm_timer();
    if (item->cmd == TUYA_MCU_CMD_WRITE && g_tx_callback != NULL) {
        g_tx_callback(item->dpid);
    }
    return 0;
}

static void tx_kick(void) {
    while (!g_tx_busy && g_txq_count > 0) {
        tx_item_t item = g_txq[g_txq_head];
        g_txq_head = (uint8_t)((g_txq_head + 1) % TX_QUEUE_LEN);
        g_txq_count--;
        if (tx_send_now(&item) != 0) {
            /* The UART would not take it (the write already spun up to its
             * limit): retry the rest later, not again in this pass. */
            tx_arm_timer();
            break;
        }
    }
}

/* Every reported DP: a report of the DP in flight is the MCU's answer to it;
* for a command we started (query-all) the first report starts the answer. */
static void tx_note_report(uint8_t dpid) {
    if (!g_tx_busy ||
        (g_tx_in_flight.cmd == TUYA_MCU_CMD_WRITE && dpid != g_tx_in_flight.dpid)) {
        return;
    }
    g_tx_busy = 0;
    hal_tasks_unschedule(&g_tx_timer);
}

/* A command we started that the MCU answers with the same command (e.g. the
 * firmware version query). */
static void tx_note_command_answer(uint8_t cmd) {
    if (!g_tx_busy || g_tx_in_flight.cmd == TUYA_MCU_CMD_WRITE || g_tx_in_flight.cmd != cmd) {
        return;
    }
    g_tx_busy = 0;
    hal_tasks_unschedule(&g_tx_timer);
}

static int tx_enqueue(uint8_t cmd, uint8_t dpid, uint8_t dp_type,
                      const void *value, uint16_t value_len) {
    tx_item_t item;

    if (!g_enabled || value_len > sizeof(item.value) || (value_len > 0 && value == NULL)) {
        return -1;
    }
    item.cmd     = cmd;
    item.dpid    = dpid;
    item.dp_type = dp_type;
    item.len     = (uint8_t)value_len;
    if (value_len > 0) {
        memcpy(item.value, value, value_len);
    }

    /* A newer write of something still waiting replaces it and moves to the
     * tail, so the MCU sees the latest writes in the order they were made
     * (off, on at 80, off must not become: off, level 80). */
    for (uint8_t i = 0; i < g_txq_count; i++) {
        tx_item_t *queued = &g_txq[(g_txq_head + i) % TX_QUEUE_LEN];
        if (queued->cmd == cmd && tx_item_id(queued) == tx_item_id(&item)) {
            for (uint8_t j = i; j + 1 < g_txq_count; j++) {
                g_txq[(g_txq_head + j) % TX_QUEUE_LEN] =
                    g_txq[(g_txq_head + j + 1) % TX_QUEUE_LEN];
            }
            g_txq_count--;
            break;
        }
    }

    if (!g_tx_busy && g_txq_count == 0) {
        return tx_send_now(&item);
    }
    if (g_txq_count >= TX_QUEUE_LEN) {
        printf("Secondary MCU TX queue full, dropping %d\r\n", tx_item_id(&item));
        return -1;
    }
    g_txq[(g_txq_head + g_txq_count) % TX_QUEUE_LEN] = item;
    g_txq_count++;
    return 0;
}

int tuya_secondary_mcu_write_dp(uint8_t dpid, uint8_t dp_type,
                                const void *value, uint16_t value_len) {
    return tx_enqueue(TUYA_MCU_CMD_WRITE, dpid, dp_type, value, value_len);
}

int tuya_secondary_mcu_queue_cmd(uint8_t cmd, const uint8_t *payload, uint16_t len) {
    return tx_enqueue(cmd, 0, 0, payload, len);
}

void tuya_secondary_mcu_set_tx_callback(tuya_secondary_mcu_tx_callback_t callback) {
    g_tx_callback = callback;
}

/* ---- RX ------------------------------------------------------------------- */

#define MAX_DP_LISTENERS    4
static tuya_secondary_mcu_dp_listener_t g_listeners[MAX_DP_LISTENERS];
static uint8_t g_listener_cnt = 0;
static tuya_secondary_mcu_command_callback_t g_command_callback = NULL;

int tuya_secondary_mcu_add_dp_listener(tuya_secondary_mcu_dp_listener_t listener) {
    for (uint8_t i = 0; i < g_listener_cnt; i++) {
        if (g_listeners[i] == listener) {
            return 0;
        }
    }
    if (listener == NULL || g_listener_cnt >= MAX_DP_LISTENERS) {
        return -1;
    }
    g_listeners[g_listener_cnt++] = listener;
    return 0;
}

void tuya_secondary_mcu_set_command_callback(tuya_secondary_mcu_command_callback_t callback) {
    g_command_callback = callback;
}

uint8_t tuya_secondary_mcu_report_is_passive(void) {
    return g_report_is_passive;
}

/* A full status dump of a two channel dimmer (13 DPs) is ~100 bytes. */
#define RX_CAPACITY            256

/* Silence after which an incomplete frame is given up (lost DMA chunk, ring
 * overflow, line glitch). Bytes of one frame arrive back to back, ~1 ms apart
 * even at 9600 baud. */
#define RX_FRAME_TIMEOUT_MS    50

static uint8_t  g_rx[RX_CAPACITY];
static uint16_t g_rx_len     = 0;
static uint32_t g_rx_last_ms = 0;

static void rx_drop(uint16_t n) {
    if (n > g_rx_len) {
        n = g_rx_len;
    }
    memmove(g_rx, g_rx + n, g_rx_len - n);
    g_rx_len -= n;
}

/* Waiting for the rest of the frame at the head: if nothing came for a while,
 * drop its sync bytes so it cannot swallow the next frame. */
static uint8_t rx_drop_if_stale(void) {
    if (g_rx_len == 0 || (hal_millis() - g_rx_last_ms) < RX_FRAME_TIMEOUT_MS) {
        return 0;
    }
    rx_drop(2);
    return 1;
}

static void rx_dispatch_report(uint8_t cmd, uint16_t frame_len) {
    uint16_t off = FRAME_HEADER_LEN;
    uint16_t end = (uint16_t)(frame_len - 1);

    /* One report may carry several DPs back to back. */
    g_report_is_passive = cmd == TUYA_MCU_CMD_REPORT_PASSIVE;
    while (off + 4 <= end) {
        uint8_t  dpid  = g_rx[off];
        uint8_t  dtype = g_rx[off + 1];
        uint16_t vlen  = (uint16_t)((g_rx[off + 2] << 8) | g_rx[off + 3]);
        if (off + 4 + vlen > end) {
            break; /* truncated unit */
        }
        const uint8_t *value = &g_rx[off + 4];
        tx_note_report(dpid);
        for (uint8_t i = 0; i < g_listener_cnt; i++) {
            g_listeners[i](dpid, dtype, value, vlen);
        }
        off = (uint16_t)(off + 4 + vlen);
    }
    g_report_is_passive = 0;
}

static void rx_process(void) {
    for (;;) {
        /* Resync on the 55 AA header. */
        while (g_rx_len >= 2 && (g_rx[0] != 0x55 || g_rx[1] != 0xAA)) {
            rx_drop(1);
        }
        if (g_rx_len < FRAME_HEADER_LEN) {
            if (rx_drop_if_stale()) {
                continue;
            }
            return;
        }

        uint16_t dlen = (uint16_t)((g_rx[6] << 8) | g_rx[7]);
        /* 32 bit: in 16 bit, 8 + 0xFFF7 + 1 wraps to 0 and passes the check. */
        uint32_t frame_len32 = FRAME_HEADER_LEN + (uint32_t)dlen + 1u;
        if (frame_len32 > RX_CAPACITY) {
            rx_drop(2);
            continue;
        }
        uint16_t frame_len = (uint16_t)frame_len32;
        if (g_rx_len < frame_len) {
            if (rx_drop_if_stale()) {
                continue;
            }
            return;
        }

        uint8_t cmd = g_rx[5];
        if (g_rx[2] != FRAME_VERSION ||
            checksum(g_rx, (uint16_t)(frame_len - 1)) != g_rx[frame_len - 1]) {
            /* Damaged, or not a frame after all: skip only the sync bytes, a
             * real frame may start inside. */
            rx_drop(2);
            continue;
        }

        if (cmd == TUYA_MCU_CMD_REPORT || cmd == TUYA_MCU_CMD_REPORT_PASSIVE) {
            rx_dispatch_report(cmd, frame_len);
        } else {
            uint16_t seq = (uint16_t)((g_rx[3] << 8) | g_rx[4]);
            tx_note_command_answer(cmd);
            if (g_command_callback != NULL) {
                g_command_callback(cmd, seq, &g_rx[FRAME_HEADER_LEN], dlen);
            }
        }
        rx_drop(frame_len);

        /* Listeners may have queued writes; an answer may have freed the slot. */
        tx_kick();
    }
}

void tuya_secondary_mcu_poll(void) {
    if (!g_enabled) {
        return;
    }
    while (g_rx_len < RX_CAPACITY && hal_uart_rx_available() > 0) {
        uint16_t read_len = 0;
        if (hal_uart_read(g_rx + g_rx_len, (uint16_t)(RX_CAPACITY - g_rx_len),
                          &read_len) != HAL_UART_OK || read_len == 0) {
            break;
        }
        g_rx_len     = (uint16_t)(g_rx_len + read_len);
        g_rx_last_ms = hal_millis();
    }
    rx_process();
}

void tuya_secondary_mcu_init(const hal_uart_config_t *cfg) {
    if (cfg != NULL) {
        g_uart_cfg = *cfg;
    }
    g_tx_timer.handler = tx_timer_fired;
    g_tx_timer.arg     = NULL;
    hal_tasks_init(&g_tx_timer);
    hal_uart_init(cfg);
    g_enabled = 1;
}
