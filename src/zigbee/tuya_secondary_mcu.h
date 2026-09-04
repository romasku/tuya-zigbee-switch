#ifndef _TUYA_SECONDARY_MCU_H_
#define _TUYA_SECONDARY_MCU_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hal/uart.h"

/*
 * Tuya Zigbee module / secondary MCU framing as seen in issue #387
 * and the Tuya module-UART reference.
 *
 * The wire encoding is a short command frame with a 55 AA magic header.
 * For the captured Avatto dimmer logs the payload layout is:
 *
 *   55 AA 02 01 00 04 00 05 01 01 00 01 01 0F
 *   |magic| ver | seq |cmd | dlen | dpid | type | vlen | value | checksum |
 *
 *   - seq is a single 2-byte, big-endian sequence number cycling
 *     0..0xfff0. The stock firmware hardcoded 0x0100 for module->MCU frames (lazy
 *     programming?), but the protocol expects a proper incrementing sequence on
 *     both directions, which this implementation now does. There is no separate
 *     direction bit - cmd already tells you which side sent it.
 *   - cmd = 0x04 (write request, Zigbee -> MCU)
 *   - cmd = 0x06 (state report, MCU -> Zigbee; sent both as a write
 *     confirmation and whenever the physical switch/button changes state)
 *   - dlen = 2-byte big-endian payload length after the cmd field
 *   - dpid = DP identifier (1 byte)
 *   - type = DP encoding type (bool 0x01, int 0x02, enum 0x04)
 *   - value length = 2-byte big-endian length of the following value bytes
 *   - value bytes = value payload, where multi-byte integer values are stored
 *     in big-endian order inside that field.
 *   - checksum = plain sum of all preceding bytes mod 256
 */

typedef enum {
    TUYA_DP_TYPE_BOOL  = 0x01,
    TUYA_DP_TYPE_VALUE = 0x02,
    TUYA_DP_TYPE_ENUM  = 0x04,
} tuya_dp_type_t;

typedef enum {
    TUYA_MCU_CMD_WRITE  = 0x04,
    /* Report the MCU sends back after executing a command, and the form it
     * uses to answer a query-all (0x28). Carries datapoints just like 0x06. */
    TUYA_MCU_CMD_REPORT_PASSIVE = 0x05,
    TUYA_MCU_CMD_REPORT = 0x06,
} tuya_mcu_cmd_t;

typedef struct {
    uint16_t seq; /* big-endian; module always sends 0x0100, MCU increments its own */
    uint8_t  cmd;
    uint8_t  dpid;
    uint8_t  dp_type;
    uint16_t value_len;
    uint8_t  value[16];
    uint8_t  checksum;
} tuya_secondary_mcu_frame_t;

/*
 * Encode one frame for transmission to the secondary MCU.
 * Returns 0 on success or -1 on insufficient buffer / malformed input.
 */
int tuya_secondary_mcu_encode_frame(const tuya_secondary_mcu_frame_t *frame,
                                    uint8_t *out, uint16_t out_len,
                                    uint16_t *written);

/*
 * Decode a raw UART frame and extract the logical Tuya data payload.
 * Validates the trailing checksum byte; returns -1 if it does not match.
 */
int tuya_secondary_mcu_decode_frame(const uint8_t *raw, uint16_t raw_len,
                                    tuya_secondary_mcu_frame_t *frame);

/*
 * Helper used by the application to place a DP write into the UART transport.
 */
int tuya_secondary_mcu_send_dp(uint8_t dpid, uint8_t dp_type,
                               const void *value, uint16_t value_len,
                               uint8_t *out, uint16_t out_len,
                               uint16_t *written);

/**
 * Send an arbitrary command frame. Responses must echo the request's seq.
 */
/** Next sequence number for a module-initiated frame. Responses must echo
 *  the request seq instead; only use this for frames we start. */
uint16_t tuya_secondary_mcu_next_tx_seq(void);

/** True while dispatching a passive report (0x05), which is the MCU
 *  answering something we sent. A passive report is never a fresh key
 *  press, so consumers must not treat it as one. */
uint8_t tuya_secondary_mcu_report_is_passive(void);

int tuya_secondary_mcu_send_cmd(uint8_t cmd, uint16_t seq,
                                const uint8_t *payload, uint16_t len);

/**
 * Write a DP frame directly to the secondary MCU UART.
 */
int tuya_secondary_mcu_write_dp(uint8_t dpid, uint8_t dp_type,
                                const void *value, uint16_t value_len);

/**
 * Callback invoked for each DP state report (cmd 0x06) received from the
 * secondary MCU, e.g. after a physical button/switch changes state.
 */
typedef void (*tuya_secondary_mcu_dp_report_callback_t)(uint8_t dpid,
                                                        uint8_t dp_type,
                                                        const uint8_t *value,
                                                        uint16_t value_len);

/** Register the (single) DP report callback. Pass NULL to unregister. */
void tuya_secondary_mcu_register_dp_report_callback(
    tuya_secondary_mcu_dp_report_callback_t callback);

/** Current DP report callback, so a new handler can chain to the previous. */
tuya_secondary_mcu_dp_report_callback_t
tuya_secondary_mcu_get_dp_report_callback(void);

/**
 * Non-DP command from the secondary MCU. `cmd` uses the tuya_mcu_cmd_t range
 * (e.g. 0x03 leave/rejoin which is sent when the physical button is held).
 */
typedef void (*tuya_secondary_mcu_command_callback_t)(uint8_t cmd,
                                                      uint16_t seq,
                                                      const uint8_t *data,
                                                      uint16_t data_len);

/** Register the (single) non-DP command callback. Pass NULL to unregister. */
void tuya_secondary_mcu_register_command_callback(
    tuya_secondary_mcu_command_callback_t callback);

/**
 * Drain and process any bytes received from the secondary MCU. Must be
 * called periodically (e.g. from the main app tick) to receive DP reports.
 */
void tuya_secondary_mcu_poll(void);

/**
 * Initialize the secondary MCU UART path.
 */
int tuya_secondary_mcu_init(const hal_uart_config_t *cfg);

/**
 * Enable the secondary MCU path at runtime. This can also be used by boards
 * that require manual initialization after UART setup.
 */
bool tuya_secondary_mcu_is_enabled(void);
void tuya_secondary_mcu_enable(void);
void tuya_secondary_mcu_disable(void);

#endif
