#include "zigbee/tuya_dp_relay.h"

#include "base_components/relay.h"
#include "base_components/button.h"
#include "zigbee/tuya_secondary_mcu.h"
#include "zigbee/dp_attr.h"
#include "hal/timer.h"

#include <stddef.h>

extern relay_t relays[];
extern uint8_t relays_cnt;
extern button_t buttons[];
extern uint8_t buttons_cnt;
extern dp_init_entry_t dp_inits[];
extern uint8_t dp_init_cnt;

static tuya_secondary_mcu_dp_report_callback_t g_next_callback = NULL;

/* When we drive a relay ourselves the MCU echoes the new state back as a
   normal report. Without this filter that echo would look like a key press,
   so a command from the coordinator would re-trigger the bindings. */
#define DP_ECHO_SLOTS      4
#define DP_ECHO_WINDOW_MS  700

static uint8_t  g_echo_dp[DP_ECHO_SLOTS];
static uint32_t g_echo_until[DP_ECHO_SLOTS];

static void dp_echo_arm(uint8_t dpid)
{
    uint32_t now = hal_millis();
    uint8_t slot = 0;
    for (uint8_t i = 0; i < DP_ECHO_SLOTS; i++)
    {
        if (g_echo_dp[i] == dpid) { slot = i; break; }
        if (g_echo_until[i] < g_echo_until[slot]) { slot = i; }
    }
    g_echo_dp[slot] = dpid;
    g_echo_until[slot] = now + DP_ECHO_WINDOW_MS;
}

static uint8_t dp_echo_consume(uint8_t dpid)
{
    uint32_t now = hal_millis();
    for (uint8_t i = 0; i < DP_ECHO_SLOTS; i++)
    {
        if (g_echo_dp[i] == dpid && g_echo_until[i] > now)
        {
            g_echo_until[i] = 0;   // one echo per write
            return 1;
        }
    }
    return 0;
}

static void tuya_dp_relay_send(uint8_t dp_id, uint8_t state)
{
    uint8_t value = state ? 1 : 0;
    dp_echo_arm(dp_id);
    tuya_secondary_mcu_write_dp(dp_id, TUYA_DP_TYPE_BOOL, &value, sizeof(value));
}

static void tuya_dp_relay_on_report(uint8_t dpid, uint8_t dp_type,
                                    const uint8_t *value, uint16_t value_len)
{
    uint8_t handled = 0;

    /* Reflect the reported state on the matching relay (also keeps the
       coordinator in sync after a local key press). */
    if (dp_type == TUYA_DP_TYPE_BOOL && value != NULL && value_len >= 1)
    {
        for (uint8_t i = 0; i < relays_cnt; i++)
        {
            if (relays[i].dp_id != 0 && relays[i].dp_id == dpid)
            {
                relay_set_state_from_dp(&relays[i], value[0]);
                handled = 1;
                break;
            }
        }
    }

    /* This hardware has no separate key-event command: a physical touch is
       only visible as a datapoint state change. Feed it to the switch
       machinery so actions, modes and outgoing binds all work - unless it is
       the echo of a write we just made. */
    /* A passive report (0x05) is the MCU answering something we sent, and a
       bulk dump is the answer to our own query. Neither is a fresh touch:
       synthesising one fires switch actions, drives binds, and feeds the
       multi-press factory reset. */
    if (!tuya_secondary_mcu_report_is_passive() &&
        !dp_attr_bulk_dump_active() && !dp_echo_consume(dpid))
    {
        for (uint8_t i = 0; i < buttons_cnt; i++)
        {
            if (buttons[i].dp_id != 0 && buttons[i].dp_id == dpid)
            {
                if (buttons[i].on_press != NULL)
                {
                    buttons[i].on_press(buttons[i].callback_param);
                }
                if (buttons[i].on_release != NULL)
                {
                    buttons[i].on_release(buttons[i].callback_param);
                }
                handled = 1;
                break;
            }
        }
    }

    if (!handled && g_next_callback != NULL)
    {
        g_next_callback(dpid, dp_type, value, value_len);
    }
}

uint8_t tuya_dp_relay_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < relays_cnt; i++)
    {
        if (relays[i].dp_id != 0) { count++; }
    }
    for (uint8_t i = 0; i < buttons_cnt; i++)
    {
        if (buttons[i].dp_id != 0) { count++; }
    }
    return count + dp_init_cnt;
}

void tuya_dp_apply_inits(void)
{
    for (uint8_t i = 0; i < dp_init_cnt; i++)
    {
        tuya_secondary_mcu_write_dp(dp_inits[i].dpid, dp_inits[i].dp_type,
                                    dp_inits[i].value, dp_inits[i].len);
    }
}

void tuya_dp_relay_init(void)
{
    if (tuya_dp_relay_count() == 0)
    {
        return;
    }

    relay_dp_send_hook = tuya_dp_relay_send;

    g_next_callback = tuya_secondary_mcu_get_dp_report_callback();
    tuya_secondary_mcu_register_dp_report_callback(tuya_dp_relay_on_report);
}
