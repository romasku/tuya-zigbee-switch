#include "zigbee/tuya_secondary_mcu.h"
#include "hal/uart.h"
#include "hal/printf_selector.h"

#include <stdbool.h>
#include <string.h>

static bool g_tuya_secondary_mcu_enabled = false;

/* A write the peripheral will not accept means the link is wedged: the
   pinmux was reassigned under us, the clock moved, an earlier re-init left
   it odd. Silence is NOT evidence of that -- an event driven MCU is quiet
   by design, and a timer based watchdog would churn the UART for nothing.
   Repeated write failures are, so that is the trigger. */
#define UART_FAILURES_BEFORE_REINIT    3

static hal_uart_config_t g_uart_config = {0};
static uint8_t           uart_write_failures = 0;

static void tuya_uart_note_write(int ok)
{
  if (ok)
  {
    uart_write_failures = 0;
    return;
  }
  if (++uart_write_failures < UART_FAILURES_BEFORE_REINIT) { return; }
  printf("Secondary MCU link stuck, reinitialising UART\r\n");
  uart_write_failures = 0;
  hal_uart_deinit();
  hal_uart_init(&g_uart_config);
}

// Outgoing sequence number, cycling 0..0xfff0 per the Tuya protocol. The
// original firmware hardcoded 0x0100 for module->MCU frames, but the protocol
// expects a proper incrementing sequence on both directions.
static uint16_t g_tx_seq = 0;
/* Set while a 0x05 report is being dispatched, see the header. */
static uint8_t g_report_is_passive = 0;

uint16_t tuya_secondary_mcu_next_tx_seq(void)
{
  uint16_t seq = g_tx_seq;
  g_tx_seq = (g_tx_seq >= 0xFFF0) ? 0 : (uint16_t)(g_tx_seq + 1);
  return seq;
}

uint8_t tuya_secondary_mcu_report_is_passive(void)
{
  return g_report_is_passive;
}

static uint8_t tuya_checksum(const uint8_t *buf, uint16_t len)
{
  // Checksum is the plain sum of all preceding bytes mod 256
  uint8_t sum = 0;

  for (uint16_t i = 0; i < len; i++)
  {
    sum += buf[i];
  }
  return sum;
}

int tuya_secondary_mcu_encode_frame(const tuya_secondary_mcu_frame_t *frame,
                                    uint8_t *out, uint16_t out_len,
                                    uint16_t *written)
{
  if (!frame || !out || out_len < 12)
  {
    return -1;
  }

  uint16_t data_len = 1 + 1 + 2 + frame->value_len;
  uint8_t frame_buf[64];
  uint16_t idx = 0;

  frame_buf[idx++] = 0x55;
  frame_buf[idx++] = 0xAA;
  frame_buf[idx++] = 0x02;
  frame_buf[idx++] = (uint8_t)(frame->seq >> 8);
  frame_buf[idx++] = (uint8_t)frame->seq;

  if (out_len < 11 + frame->value_len)
  {
    return -1;
  }

  frame_buf[idx++] = (uint8_t)frame->cmd;
  frame_buf[idx++] = (uint8_t)(data_len >> 8);
  frame_buf[idx++] = (uint8_t)(data_len & 0xFF);
  frame_buf[idx++] = frame->dpid;
  frame_buf[idx++] = frame->dp_type;
  frame_buf[idx++] = (uint8_t)((frame->value_len >> 8) & 0xFF);
  frame_buf[idx++] = (uint8_t)(frame->value_len & 0xFF);

  for (uint16_t i = 0; i < frame->value_len; i++)
  {
    frame_buf[idx++] = frame->value[i];
  }

  // Compute the checksum length before incrementing idx: combining both in
  // one expression is unsequenced (undefined which value idx has when the
  // checksum call is evaluated).
  uint8_t checksum = tuya_checksum(frame_buf, idx);
  frame_buf[idx++] = checksum;

  if (idx > out_len)
  {
    return -1;
  }

  memcpy(out, frame_buf, idx);
  if (written)
  {
    *written = idx;
  }

  return 0;
}

int tuya_secondary_mcu_decode_frame(const uint8_t *raw, uint16_t raw_len,
                                    tuya_secondary_mcu_frame_t *frame)
{
  /* Smallest valid frame is header(8) + checksum(1); command frames such as
     0x20 "query network status" carry no payload at all. The previous
     implementation required 12 bytes and unconditionally parsed a datapoint
     header, so every non-DP command was silently dropped. */
  if (!raw || !frame || raw_len < 9)
  {
    return -1;
  }

  memset(frame, 0, sizeof(*frame));

  if (raw[0] != 0x55 || raw[1] != 0xAA || raw[2] != 0x02)
  {
    return -1;
  }

  /* 55 AA 02 <seq_hi> <seq_lo> <cmd> <dlen_hi> <dlen_lo> <data...> <chk> */
  frame->seq = (uint16_t)((raw[3] << 8) | raw[4]);
  frame->cmd = raw[5];

  uint16_t dlen = (uint16_t)((raw[6] << 8) | raw[7]);
  uint16_t payload_off = 8;

  if (raw_len < (uint16_t)(payload_off + dlen + 1))
  {
    return -1;
  }

  frame->checksum = raw[payload_off + dlen];
  if (frame->checksum != tuya_checksum(raw, (uint16_t)(payload_off + dlen)))
  {
    return -1;
  }

  if (frame->cmd == TUYA_MCU_CMD_REPORT || frame->cmd == TUYA_MCU_CMD_WRITE ||
      frame->cmd == TUYA_MCU_CMD_REPORT_PASSIVE)
  {
    /* Datapoint frame: dpid | type | len(2) | value */
    if (dlen < 4)
    {
      return -1;
    }
    frame->dpid = raw[payload_off];
    frame->dp_type = raw[payload_off + 1];
    frame->value_len = (uint16_t)((raw[payload_off + 2] << 8) | raw[payload_off + 3]);
    if (frame->value_len > sizeof(frame->value) ||
        frame->value_len > (uint16_t)(dlen - 4))
    {
      return -1;
    }
    if (frame->value_len > 0)
    {
      memcpy(frame->value, &raw[payload_off + 4], frame->value_len);
    }
  }
  else
  {
    /* Generic command frame: the payload is raw bytes. */
    frame->dpid = 0;
    frame->dp_type = 0;
    frame->value_len = dlen > sizeof(frame->value) ? sizeof(frame->value) : dlen;
    if (frame->value_len > 0)
    {
      memcpy(frame->value, &raw[payload_off], frame->value_len);
    }
  }

  return 0;
}

int tuya_secondary_mcu_send_dp(uint8_t dpid, uint8_t dp_type,
                               const void *value, uint16_t value_len,
                               uint8_t *out, uint16_t out_len,
                               uint16_t *written)
{
  tuya_secondary_mcu_frame_t frame;

  memset(&frame, 0, sizeof(frame));

  frame.seq = g_tx_seq;
  /* Tuya MCU sequence: +1 each frame, wrap from 0xFFF0 back to 0x0000 */
  g_tx_seq = (g_tx_seq >= 0xFFF0) ? 0 : (uint16_t)(g_tx_seq + 1);
  frame.cmd = TUYA_MCU_CMD_WRITE;
  frame.dpid = dpid;
  frame.dp_type = dp_type;
  frame.value_len = value_len;
  memcpy(frame.value, value, value_len);

  return tuya_secondary_mcu_encode_frame(&frame, out, out_len, written);
}

bool tuya_secondary_mcu_is_enabled(void)
{
  return g_tuya_secondary_mcu_enabled;
}

void tuya_secondary_mcu_enable(void)
{
  g_tuya_secondary_mcu_enabled = true;
}

void tuya_secondary_mcu_disable(void)
{
  g_tuya_secondary_mcu_enabled = false;
}

int tuya_secondary_mcu_init(const hal_uart_config_t *cfg)
{
  if (cfg != NULL) { g_uart_config = *cfg; }
  hal_uart_init(cfg);
  tuya_secondary_mcu_enable();
  return 0;
}

int tuya_secondary_mcu_send_cmd(uint8_t cmd, uint16_t seq,
                                const uint8_t *payload, uint16_t len)
{
  if (!tuya_secondary_mcu_is_enabled() || len > 32)
  {
    return -1;
  }

  uint8_t buf[48];
  uint16_t idx = 0;
  buf[idx++] = 0x55;
  buf[idx++] = 0xAA;
  buf[idx++] = 0x02;
  buf[idx++] = (uint8_t)(seq >> 8);
  buf[idx++] = (uint8_t)seq;
  buf[idx++] = cmd;
  buf[idx++] = (uint8_t)(len >> 8);
  buf[idx++] = (uint8_t)len;
  for (uint16_t i = 0; i < len; i++)
  {
    buf[idx++] = payload[i];
  }
  uint8_t chk = tuya_checksum(buf, idx);
  buf[idx++] = chk;

  int ok = (hal_uart_write(buf, idx, NULL) == HAL_UART_OK);
  tuya_uart_note_write(ok);
  return ok ? 0 : -1;
}

int tuya_secondary_mcu_write_dp(uint8_t dpid, uint8_t dp_type,
                                const void *value, uint16_t value_len)
{
  if (!tuya_secondary_mcu_is_enabled())
  {
    return -1;
  }

  uint8_t buffer[64];
  uint16_t written = 0;
  int status = tuya_secondary_mcu_send_dp(dpid, dp_type, value, value_len,
                                          buffer, sizeof(buffer), &written);
  if (status != 0)
  {
    return status;
  }
  int ok = (hal_uart_write(buffer, written, NULL) == HAL_UART_OK);
  tuya_uart_note_write(ok);
  return ok ? 0 : -1;
}

static tuya_secondary_mcu_dp_report_callback_t g_dp_report_callback = NULL;
static tuya_secondary_mcu_command_callback_t g_command_callback = NULL;

void tuya_secondary_mcu_register_dp_report_callback(
    tuya_secondary_mcu_dp_report_callback_t callback)
{
  g_dp_report_callback = callback;
}

tuya_secondary_mcu_dp_report_callback_t
tuya_secondary_mcu_get_dp_report_callback(void)
{
  return g_dp_report_callback;
}

void tuya_secondary_mcu_register_command_callback(
    tuya_secondary_mcu_command_callback_t callback)
{
  g_command_callback = callback;
}

// Assembly buffer for reconstructing frames arriving byte-by-byte over UART.
#define TUYA_RX_ASSEMBLY_CAPACITY 64
static uint8_t g_rx_assembly[TUYA_RX_ASSEMBLY_CAPACITY];
static uint16_t g_rx_assembly_len = 0;

static void tuya_secondary_mcu_process_assembly(void)
{
  for (;;)
  {
    // Resync on the 55 AA magic header, discarding stray bytes.
    while (g_rx_assembly_len >= 2 &&
           (g_rx_assembly[0] != 0x55 || g_rx_assembly[1] != 0xAA))
    {
      memmove(g_rx_assembly, g_rx_assembly + 1, --g_rx_assembly_len);
    }

    // Need the fixed 8-byte header (incl. dlen) to know the full frame size.
    if (g_rx_assembly_len < 8)
    {
      return;
    }

    uint16_t dlen = (uint16_t)((g_rx_assembly[6] << 8) | g_rx_assembly[7]);
    uint16_t frame_len = 8 + dlen + 1; // header + payload + checksum

    if (frame_len > TUYA_RX_ASSEMBLY_CAPACITY)
    {
      // Corrupt/oversized frame: drop the sync bytes and try to resync.
      memmove(g_rx_assembly, g_rx_assembly + 2, g_rx_assembly_len - 2);
      g_rx_assembly_len -= 2;
      continue;
    }

    if (g_rx_assembly_len < frame_len)
    {
      return; // wait for the rest of the frame
    }

    tuya_secondary_mcu_frame_t frame;
    int decode_status =
        tuya_secondary_mcu_decode_frame(g_rx_assembly, frame_len, &frame);

    // Consume this frame regardless of decode success, so a checksum
    // mismatch can't get us stuck resyncing on the same bytes forever.
    memmove(g_rx_assembly, g_rx_assembly + frame_len,
            g_rx_assembly_len - frame_len);
    g_rx_assembly_len -= frame_len;

    if (decode_status == 0)
    {
      if (frame.cmd == TUYA_MCU_CMD_REPORT ||
          frame.cmd == TUYA_MCU_CMD_REPORT_PASSIVE)
      {
        g_report_is_passive =
            (frame.cmd == TUYA_MCU_CMD_REPORT_PASSIVE) ? 1 : 0;
        // DP state report (button press, write ack), e.g. physical button.
        if (g_dp_report_callback != NULL)
        {
          g_dp_report_callback(frame.dpid, frame.dp_type, frame.value,
                               frame.value_len);
        }
        g_report_is_passive = 0;
      }
      else
      {
        // Other MCU-originated commands (0x02 idle/network query, 0x03 leave
        // network / rejoin, ...). Let the application decide.
        if (g_command_callback != NULL)
        {
          g_command_callback(frame.cmd, frame.seq, frame.value, frame.value_len);
        }
      }
    }
  }
}

void tuya_secondary_mcu_poll(void)
{
  if (!tuya_secondary_mcu_is_enabled())
  {
    return;
  }

  while (g_rx_assembly_len < TUYA_RX_ASSEMBLY_CAPACITY &&
         hal_uart_rx_available() > 0)
  {
    uint16_t read_len = 0;
    hal_uart_status_t st =
        hal_uart_read(g_rx_assembly + g_rx_assembly_len,
                      (uint16_t)(TUYA_RX_ASSEMBLY_CAPACITY - g_rx_assembly_len),
                      &read_len);
    if (st != HAL_UART_OK || read_len == 0)
    {
      break;
    }
    g_rx_assembly_len += read_len;
  }

  tuya_secondary_mcu_process_assembly();
}
