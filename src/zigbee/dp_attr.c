#include "dp_attr.h"
#include "consts.h"
#include "device_config/config_parser.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "tuya_secondary_mcu.h"
#include <string.h>

/* Command 0x28, "query all DP status", module -> MCU. An empty data field
 * means every datapoint. Note this is the Zigbee serial protocol: in the Wi-Fi
 * one the same request is 0x08, which here is the RF test command instead. */
#define TUYA_MCU_QUERY_ALL_DP    0x28

dp_attr_t dp_attrs[MAX_DP_ATTRS];
uint8_t   dp_attrs_cnt = 0;

static tuya_secondary_mcu_dp_report_callback_t g_next_callback = NULL;

/* Window during which reports are assumed to be the answer to our own
   query-all rather than someone touching the switch. */
#define BULK_DUMP_WINDOW_MS    3000
static uint8_t  g_bulk_dump_armed      = 0;
static uint32_t g_bulk_dump_started_ms = 0;

uint8_t dp_attr_bulk_dump_active(void)
{
  if (!g_bulk_dump_armed)
  {
    return 0;
  }
  /* Unsigned subtraction, so this stays correct across a millis wrap. */
  if ((hal_millis() - g_bulk_dump_started_ms) >= BULK_DUMP_WINDOW_MS)
  {
    g_bulk_dump_armed = 0;
    return 0;
  }
  return 1;
}

static int8_t hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static uint8_t hex_byte(const char *s, uint8_t *out) {
    int8_t hi = hex_digit(s[0]);
    int8_t lo = hex_digit(s[1]);

    if (hi < 0 || lo < 0) {
        return 0;
    }
    *out = (uint8_t)((hi << 4) | lo);
    return 1;
}

static uint8_t parse_entry(const char *entry, uint8_t len) {
    uint8_t dp   = 0;
    uint8_t attr = 0;

    if (len != 5) {
        return 0;
    }
    if (!hex_byte(entry, &dp) || dp == 0) {
        return 0;
    }
    if (!hex_byte(entry + 3, &attr)) {
        return 0;
    }

    dp_attrs[dp_attrs_cnt].dp_id   = dp;
    dp_attrs[dp_attrs_cnt].attr_id = (uint16_t)(0xff00 | attr);

    switch (entry[2]) {
    case 'B':
        dp_attrs[dp_attrs_cnt].dp_type  = TUYA_DP_TYPE_BOOL;
        dp_attrs[dp_attrs_cnt].zcl_type = ZCL_DATA_TYPE_BOOLEAN;
        dp_attrs[dp_attrs_cnt].zcl_size = 1;
        break;

    case 'E':
        dp_attrs[dp_attrs_cnt].dp_type  = TUYA_DP_TYPE_ENUM;
        dp_attrs[dp_attrs_cnt].zcl_type = ZCL_DATA_TYPE_ENUM8;
        dp_attrs[dp_attrs_cnt].zcl_size = 1;
        break;

    case 'V':
        dp_attrs[dp_attrs_cnt].dp_type  = TUYA_DP_TYPE_VALUE;
        dp_attrs[dp_attrs_cnt].zcl_type = ZCL_DATA_TYPE_UINT32;
        dp_attrs[dp_attrs_cnt].zcl_size = 4;
        break;

    default:
        return 0;
    }

    memset(dp_attrs[dp_attrs_cnt].raw, 0, sizeof(dp_attrs[dp_attrs_cnt].raw));
    dp_attrs_cnt++;
    return 1;
}

void dp_attr_parse(const char *str, uint16_t len) {
    uint16_t start = 0;
    uint16_t i     = 0;

    dp_attrs_cnt = 0;
    if (str == NULL) {
        return;
    }

    for (i = 0; i <= len; i++) {
        if (i == len || str[i] == ';') {
            if (i > start && dp_attrs_cnt < MAX_DP_ATTRS) {
                if (!parse_entry(str + start, (uint8_t)(i - start))) {
                    printf("dp_attr: bad entry at %d\r\n", (int)start);
                }
            }
            start = i + 1;
        }
    }
    printf("dp_attr: %d attribute(s) mapped\r\n", (int)dp_attrs_cnt);
}

uint8_t dp_attr_register(hal_zigbee_attribute *table, uint8_t start_index) {
    uint8_t i = 0;

    for (i = 0; i < dp_attrs_cnt; i++) {
        table[start_index + i].attribute_id = dp_attrs[i].attr_id;
        table[start_index + i].data_type_id = dp_attrs[i].zcl_type;
        table[start_index + i].flag         = ATTR_WRITABLE;
        table[start_index + i].size         = dp_attrs[i].zcl_size;
        table[start_index + i].value        = dp_attrs[i].raw;
    }
    return dp_attrs_cnt;
}

static dp_attr_t *find_by_attr(uint16_t attr_id) {
    uint8_t i = 0;

    for (i = 0; i < dp_attrs_cnt; i++) {
        if (dp_attrs[i].attr_id == attr_id) {
            return &dp_attrs[i];
        }
    }
    return NULL;
}

static dp_attr_t *find_by_dp(uint8_t dp_id) {
    uint8_t i = 0;

    for (i = 0; i < dp_attrs_cnt; i++) {
        if (dp_attrs[i].dp_id == dp_id) {
            return &dp_attrs[i];
        }
    }
    return NULL;
}

uint8_t dp_attr_on_write(uint16_t attr_id) {
    dp_attr_t *entry = find_by_attr(attr_id);
    uint8_t    payload[4];

    if (entry == NULL) {
        return 0;
    }

    if (entry->zcl_size == 1) {
        payload[0] = entry->raw[0];
    } else {
        /* ZCL stores little endian, the Tuya wire format is big endian. */
        payload[0] = entry->raw[3];
        payload[1] = entry->raw[2];
        payload[2] = entry->raw[1];
        payload[3] = entry->raw[0];
    }

    tuya_secondary_mcu_write_dp(entry->dp_id, entry->dp_type, payload,
                                entry->zcl_size);
    return 1;
}

static void dp_attr_on_report(uint8_t dpid, uint8_t dp_type,
                              const uint8_t *value, uint16_t value_len) {
    dp_attr_t *entry = find_by_dp(dpid);

    if (entry == NULL || value == NULL) {
        if (g_next_callback) {
            g_next_callback(dpid, dp_type, value, value_len);
        }
        return;
    }

    if (entry->zcl_size == 1 && value_len >= 1) {
        entry->raw[0] = value[0];
    } else if (entry->zcl_size == 4 && value_len >= 4) {
        entry->raw[0] = value[3];
        entry->raw[1] = value[2];
        entry->raw[2] = value[1];
        entry->raw[3] = value[0];
    } else {
        return;
    }

    hal_zigbee_notify_attribute_changed(endpoints[0].endpoint, ZCL_CLUSTER_BASIC,
                                        entry->attr_id);
}

void dp_attr_init(void) {
    if (dp_attrs_cnt == 0) {
        return;
    }
    g_next_callback = tuya_secondary_mcu_get_dp_report_callback();
    tuya_secondary_mcu_register_dp_report_callback(dp_attr_on_report);
}

void dp_attr_query_all(void) {
    if (dp_attrs_cnt == 0) {
        return;
    }
    g_bulk_dump_armed      = 1;
    g_bulk_dump_started_ms = hal_millis();
    tuya_secondary_mcu_send_cmd(TUYA_MCU_QUERY_ALL_DP,
                                tuya_secondary_mcu_next_tx_seq(), NULL, 0);
}
