#include "hal/gpio.h"
#include "nvm_items.h"
#include "hal/printf_selector.h"
#include "hal/zigbee.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/battery_cluster.h"
#include "zigbee/consts.h"
#include "zigbee/cover_cluster.h"
#include "zigbee/cover_switch_cluster.h"
#include "zigbee/group_cluster.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/poll_control_cluster.h"
#include "zigbee/switch_cluster.h"
#include "zigbee/time_cluster.h"
#include "zigbee/dimmer_cluster.h"
#include "hal/uart.h"
#include "zigbee/tuya_dp_relay.h"
#include "zigbee/dp_attr.h"

#include <stdint.h>
#include <string.h>

#include "base_components/led.h"
#include "base_components/network_indicator.h"
#include "base_components/battery.h"
#include "config_nv.h"
#include "device_config/device_params_nv.h"
#include "device_config/reset.h"
#include "hal/system.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"

// Forward declarations
void peripherals_init(void);

// Max entries for the per-dimmer pending DPID lookup arrays in parse_config().
// This is independent of (and >=) the 4-element dimmer_clusters[] capacity,
// to allow Pxx tokens to use a full hex digit pair as the dimmer index.
#define DIMMER_CONFIG_MAX_COUNT 16

// extern ota_preamble_t baseEndpoint_otaInfo;

network_indicator_t network_indicator = {
    .leds = {NULL, NULL, NULL, NULL},
    .has_dedicated_led = 0,
    .manual_state_when_connected = 1,
};

led_t leds[5];
uint8_t leds_cnt = 0;

button_t buttons[11];
uint8_t buttons_cnt = 0;

hal_uart_config_t mcu_uart_config = {0};
uint8_t g_power_on_dp_id = 0;

/* DPs written to the secondary MCU at startup (IT tokens). */
dp_init_entry_t dp_inits[12];
uint8_t dp_init_cnt = 0;

relay_t relays[10]; // 4 relay endpoints + 3 cover endpoints
uint8_t relays_cnt = 0;

zigbee_basic_cluster basic_cluster = {
    .deviceEnable = 1,
};

zigbee_group_cluster group_cluster = {};

zigbee_switch_cluster switch_clusters[4];
uint8_t switch_clusters_cnt = 0;

zigbee_relay_cluster relay_clusters[6];
uint8_t relay_clusters_cnt = 0;

zigbee_dimmer_cluster dimmer_clusters[4];
uint8_t dimmer_clusters_cnt = 0;

zigbee_cover_switch_cluster cover_switch_clusters[3];
uint8_t cover_switch_clusters_cnt = 0;

zigbee_cover_cluster cover_clusters[3];
uint8_t cover_clusters_cnt = 0;

hal_zigbee_cluster clusters[64];
hal_zigbee_endpoint endpoints[10];

uint8_t allow_simultaneous_latching_pulses = 0;

battery_t battery = {
    .pin = HAL_INVALID_PIN,
    .voltage_min = 2000,
    .voltage_max = 3000,
};

uint32_t parse_int(const char *s);
uint8_t parse_hex_nibble(char c);
bool parse_hex_byte(const char *s, uint8_t *out);
char *seek_until(char *cursor, char needle);
char *extract_next_entry(char **cursor);

void on_reset_clicked(void *_)
{
    hal_factory_reset();
}

void on_multi_press_reset(void *_, uint8_t press_count)
{
    if (g_multi_press_reset_count != 0 &&
        press_count >= g_multi_press_reset_count)
    {
        hal_factory_reset();
    }
}

void parse_config()
{
    device_config_read_from_nv();
    /* Datapoint map lives in its own string: the ZCL write path caps a single
       string at ~74 characters and the pin config already fills it. */
    dp_config_read_from_nv();
    dp_attr_parse((const char *)dp_config_str.data, dp_config_str.size);
    /* device_config itself can also run past a single ~74-char write, e.g. a
       board declaring six switch AND six relay endpoints. The overflow
       tokens are appended here, in RAM, before any tokenizing below --
       parse_config sees one string either way. */
    device_config_ext_read_from_nv();
    device_config_append_ext();
    char *cursor = (char *)device_config_str.data;

    const char *zb_manufacturer = extract_next_entry(&cursor);

    basic_cluster.manuName[0] = strlen(zb_manufacturer);
    if (basic_cluster.manuName[0] > 31)
    {
        printf("Manufacturer too big\r\n");
        reset_all();
    }
    memcpy(basic_cluster.manuName + 1, zb_manufacturer,
           basic_cluster.manuName[0]);

    const char *zb_model = extract_next_entry(&cursor);
    basic_cluster.modelId[0] = strlen(zb_model);
    if (basic_cluster.modelId[0] > 31)
    {
        printf("Model too big\r\n");
        reset_all();
    }
    memcpy(basic_cluster.modelId + 1, zb_model, basic_cluster.modelId[0]);

    bool has_dedicated_status_led = false;
    uint16_t debounce_ms = DEBOUNCE_DELAY_MS;
    // Indexed by dimmer sequence index (0-based, matching Pxx and DM<N> order),
    // NOT the Zigbee endpoint number, which is assigned later.
    uint8_t dimmer_onoff_dpid[DIMMER_CONFIG_MAX_COUNT] = {0};
    uint8_t dimmer_level_dpid[DIMMER_CONFIG_MAX_COUNT] = {0};
    uint8_t dimmer_switch_type_dpid[DIMMER_CONFIG_MAX_COUNT] = {0};
    uint8_t dimmer_min_level_dpid[DIMMER_CONFIG_MAX_COUNT] = {0};
    uint8_t dimmer_max_level_dpid[DIMMER_CONFIG_MAX_COUNT] = {0};
    uint8_t dimmer_power_on_behavior_dpid = 0;
    char *entry;
    for (entry = extract_next_entry(&cursor); *entry != '\0';
         entry = extract_next_entry(&cursor))
    {
        if (entry[0] == 'S' && entry[1] == 'L' && entry[2] == 'P')
        {
            // Simultaneous Latching Pulses == SLP
            allow_simultaneous_latching_pulses = 1;
        }
        else if (entry[0] == 'D' && entry[1] >= '0' && entry[1] <= '9')
        {
            // D<N> sets the global debounce duration in milliseconds.
            debounce_ms = (uint16_t)parse_int(entry + 1);
            for (int i = 0; i < buttons_cnt; i++)
            {
                buttons[i].debounce_delay_ms = debounce_ms;
            }
        }
        else if (entry[0] == 'B' && entry[1] == 'T')
        {
            // Battery: BT<pin>, e.g. BTC5
            hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 2);
            battery.pin = pin;
            battery_init(&battery);
        }
        else if (entry[0] == 'B')
        {
            hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_pull_t pull = hal_gpio_parse_pull(entry + 3);
            hal_gpio_init(pin, 1, pull);

            buttons[buttons_cnt].pin = pin;
            buttons[buttons_cnt].long_press_duration_ms = 2000;
            buttons[buttons_cnt].multi_press_duration_ms = 800;
            buttons[buttons_cnt].debounce_delay_ms = debounce_ms;
            buttons[buttons_cnt].on_long_press = on_reset_clicked;
            buttons_cnt++;
        }
        else if (entry[0] == 'L')
        {
            hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);
            leds[leds_cnt].pin = pin;
            leds[leds_cnt].on_high = entry[3] != 'i';

            led_init(&leds[leds_cnt]);

            network_indicator.leds[0] = &leds[leds_cnt];
            network_indicator.leds[1] = NULL;
            network_indicator.has_dedicated_led = true;

            has_dedicated_status_led = true;
            leds_cnt++;
        }
        else if (entry[0] == 'I' && entry[1] >= 'A' && entry[1] <= 'D')
        {
            hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);
            leds[leds_cnt].pin = pin;
            leds[leds_cnt].on_high = entry[3] != 'i';
            led_init(&leds[leds_cnt]);

            for (int index = 0; index < 4; index++)
            {
                if (relay_clusters[index].indicator_led == NULL)
                {
                    relay_clusters[index].indicator_led = &leds[leds_cnt];
                    break;
                }
            }

            for (int index = 0; index < 4; index++)
            {
                if (switch_clusters[index].indicator_led == NULL)
                {
                    switch_clusters[index].indicator_led = &leds[leds_cnt];
                    break;
                }
            }

            if (!has_dedicated_status_led)
            {
                for (int index = 0; index < 4; index++)
                {
                    if (network_indicator.leds[index] == NULL)
                    {
                        network_indicator.leds[index] = &leds[leds_cnt];
                        break;
                    }
                }
            }
            leds_cnt++;
        }
        else if (entry[0] == 'S' && entry[1] >= 'A' && entry[1] <= 'D')
        {
            hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_pull_t pull = hal_gpio_parse_pull(entry + 3);
            hal_gpio_init(pin, 1, pull);

            buttons[buttons_cnt].pin = pin;
            buttons[buttons_cnt].long_press_duration_ms = 800;
            buttons[buttons_cnt].multi_press_duration_ms = 800;
            buttons[buttons_cnt].debounce_delay_ms = debounce_ms;
            buttons[buttons_cnt].on_multi_press = on_multi_press_reset;

            if (entry[3] == 'd')
                buttons[buttons_cnt].pressed_when_high = 1;
            switch_clusters[switch_clusters_cnt].switch_idx = switch_clusters_cnt;
            switch_clusters[switch_clusters_cnt].mode =
                ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE;
            switch_clusters[switch_clusters_cnt].action =
                ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE;
            switch_clusters[switch_clusters_cnt].relay_mode =
                ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT;
            switch_clusters[switch_clusters_cnt].binded_mode =
                ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT;
            switch_clusters[switch_clusters_cnt].relay_index = switch_clusters_cnt + 1;
            switch_clusters[switch_clusters_cnt].button = &buttons[buttons_cnt];
            switch_clusters[switch_clusters_cnt].level_move_rate = 50;
            buttons_cnt++;
            switch_clusters_cnt++;
        }
        else if (entry[0] == 'S' && entry[1] == 'T')
        {
            // ST<hh> - touch/scene event arriving as a Tuya datapoint report.
            // Creates a normal switch endpoint, so binds/detached/action modes
            // all work exactly as they do for a GPIO button.
            uint8_t dpid = 0;
            if (parse_hex_byte(entry + 2, &dpid) && dpid != 0)
            {
                buttons[buttons_cnt].pin = HAL_INVALID_PIN;
                buttons[buttons_cnt].dp_id = dpid;
                buttons[buttons_cnt].long_press_duration_ms = 800;
                buttons[buttons_cnt].multi_press_duration_ms = 800;
                buttons[buttons_cnt].on_multi_press = on_multi_press_reset;

                switch_clusters[switch_clusters_cnt].switch_idx = switch_clusters_cnt;
                switch_clusters[switch_clusters_cnt].mode =
                    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY;
                switch_clusters[switch_clusters_cnt].action =
                    ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE;
                switch_clusters[switch_clusters_cnt].relay_mode =
                    ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED;
                switch_clusters[switch_clusters_cnt].binded_mode =
                    ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT;
                switch_clusters[switch_clusters_cnt].relay_index = switch_clusters_cnt + 1;
                switch_clusters[switch_clusters_cnt].button = &buttons[buttons_cnt];
                switch_clusters[switch_clusters_cnt].level_move_rate = 50;
                buttons_cnt++;
                switch_clusters_cnt++;
            }
        }
        else if (entry[0] == 'I' && entry[1] == 'T')
        {
            // IT<hh><t><v..> - write a datapoint to the secondary MCU at boot.
            // t: 1=bool 4=enum (1 byte value), 2=value (4 bytes, big-endian).
            uint8_t dpid = 0;
            if (parse_hex_byte(entry + 2, &dpid) && dpid != 0 &&
                dp_init_cnt < (uint8_t)(sizeof(dp_inits) / sizeof(dp_inits[0])))
            {
                char t = entry[4];
                uint8_t ok = 0;
                dp_inits[dp_init_cnt].dpid = dpid;
                if (t == '1' || t == '4')
                {
                    uint8_t v = 0;
                    if (parse_hex_byte(entry + 5, &v))
                    {
                        dp_inits[dp_init_cnt].dp_type = (t == '1') ? 0x01 : 0x04;
                        dp_inits[dp_init_cnt].len = 1;
                        dp_inits[dp_init_cnt].value[0] = v;
                        ok = 1;
                    }
                }
                else if (t == '2')
                {
                    uint8_t b0, b1, b2, b3;
                    if (parse_hex_byte(entry + 5, &b0) && parse_hex_byte(entry + 7, &b1) &&
                        parse_hex_byte(entry + 9, &b2) && parse_hex_byte(entry + 11, &b3))
                    {
                        dp_inits[dp_init_cnt].dp_type = 0x02;
                        dp_inits[dp_init_cnt].len = 4;
                        dp_inits[dp_init_cnt].value[0] = b0;
                        dp_inits[dp_init_cnt].value[1] = b1;
                        dp_inits[dp_init_cnt].value[2] = b2;
                        dp_inits[dp_init_cnt].value[3] = b3;
                        ok = 1;
                    }
                }
                if (ok) { dp_init_cnt++; }
            }
        }
        else if (entry[0] == 'R' && entry[1] == 'T')
        {
            // RT<hh> - relay driven through the Tuya secondary MCU.
            // <hh> = datapoint id in hex (RT18 -> DP 24).
            uint8_t dpid = 0;
            if (parse_hex_byte(entry + 2, &dpid) && dpid != 0)
            {
                if (relay_clusters_cnt >= MAX_RELAYS || relays_cnt >= MAX_RELAYS)
                {
                    printf("Too many relays, ignoring %s\r\n", entry);
                    continue;
                }
                relays[relays_cnt].pin = HAL_INVALID_PIN;
                relays[relays_cnt].off_pin = HAL_INVALID_PIN;
                relays[relays_cnt].on_high = 1;
                relays[relays_cnt].is_latching = 0;
                relays[relays_cnt].dp_id = dpid;

                // RT<state><countdown> - the second pair is optional
                uint8_t cdp = 0;
                if (entry[4] != '\0' && parse_hex_byte(entry + 4, &cdp))
                {
                    relays[relays_cnt].countdown_dp_id = cdp;
                }

                relay_clusters[relay_clusters_cnt].relay_idx = relay_clusters_cnt;
                relay_clusters[relay_clusters_cnt].relay = &relays[relays_cnt];

                relays_cnt++;
                relay_clusters_cnt++;
            }
        }
        else if (entry[0] == 'P' && entry[1] == 'T')
        {
            // PT<hh> - device-wide power-on-behaviour datapoint
            uint8_t dp = 0;
            if (parse_hex_byte(entry + 2, &dp)) { g_power_on_dp_id = dp; }
        }
        else if (entry[0] == 'W')
        {
            // W<tx><rx> - UART pins towards the secondary MCU.
            // TLSR8258 pinmux: TX = A2 B1 C2 D0 D3 D7 / RX = A0 B0 B7 C3 C5 D6
            mcu_uart_config.tx_pin = hal_gpio_parse_pin(entry + 1);
            mcu_uart_config.rx_pin = hal_gpio_parse_pin(entry + 3);
        }
        else if (entry[0] == 'Y')
        {
            // Y<n> - baudrate of that UART (default 115200)
            mcu_uart_config.baudrate = parse_int(entry + 1);
        }
        else if (entry[0] == 'R' && entry[1] >= 'A' && entry[1] <= 'D')
        {
            if (relay_clusters_cnt >= MAX_RELAYS || relays_cnt >= MAX_RELAYS)
            {
                printf("Too many relays, ignoring %s\r\n", entry);
                continue;
            }
            hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);

            relays[relays_cnt].pin = pin;
            relays[relays_cnt].on_high = 1;

            if (entry[3] != '\0')
            {
                pin = hal_gpio_parse_pin(entry + 3);
                hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);
                relays[relays_cnt].off_pin = pin;
                relays[relays_cnt].is_latching = 1;
            }

            relay_clusters[relay_clusters_cnt].relay_idx = relay_clusters_cnt;
            relay_clusters[relay_clusters_cnt].relay = &relays[relays_cnt];

            relays_cnt++;
            relay_clusters_cnt++;
        }
        else if (entry[0] == 'D' && entry[1] == 'M')
        {
            uint8_t count = (uint8_t)parse_int(entry + 2);
            if (count > 4)
                count = 4;
            for (int i = 0; i < count; i++)
            {
                // DPID fields are resolved after the full config string has
                // been parsed (see below), since Pxx entries configuring
                // this dimmer's DPIDs may appear before or after this token.
                dimmer_clusters[dimmer_clusters_cnt].dimmer_idx = dimmer_clusters_cnt;
                dimmer_clusters[dimmer_clusters_cnt].startup_mode =
                    ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF;
                dimmer_clusters[dimmer_clusters_cnt].current_level = 0;
                dimmer_clusters[dimmer_clusters_cnt].min_level = 1;
                dimmer_clusters[dimmer_clusters_cnt].max_level = 100;
                dimmer_clusters[dimmer_clusters_cnt].switch_type =
                    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE;
                dimmer_clusters[dimmer_clusters_cnt].on = 0;
                dimmer_clusters_cnt++;
            }
        }
        else if (entry[0] == 'U')
        {
            // Universal power-on-behavior DPID, e.g. U0E = 0x0E (hex, like the
            // Pxx DPID fields). parse_int() is decimal-only and would stop at
            // the 'E', yielding 0 - so use parse_hex_byte instead.
            uint8_t dpid;
            if (parse_hex_byte(entry + 1, &dpid))
            {
                dimmer_power_on_behavior_dpid = dpid;
            }
        }
        else if (entry[0] == 'P')
        {
            uint8_t dimmer_idx;
            if (!parse_hex_byte(entry + 1, &dimmer_idx) ||
                dimmer_idx >= DIMMER_CONFIG_MAX_COUNT)
            {
                continue;
            }
            char *cursor_p = entry + 3;
            // Pxx entries map per-dimmer DPID IDs, where xx is the 0-based
            // dimmer sequence index in hex (00, 01, ...), matching the order
            // dimmers are defined via DM<N> - NOT the Zigbee endpoint number.
            // Supported tokens inside Pxx are:
            //   O = On/Off DPID
            //   L = Level DPID
            //   S = Switch type DPID
            //   M = Min brightness DPID
            //   X = Max brightness DPID
            while (*cursor_p != '\0')
            {
                if (*cursor_p == 'O')
                {
                    uint8_t value;
                    if (parse_hex_byte(cursor_p + 1, &value))
                    {
                        dimmer_onoff_dpid[dimmer_idx] = value;
                        cursor_p += 3;
                        continue;
                    }
                }
                else if (*cursor_p == 'L')
                {
                    uint8_t value;
                    if (parse_hex_byte(cursor_p + 1, &value))
                    {
                        dimmer_level_dpid[dimmer_idx] = value;
                        cursor_p += 3;
                        continue;
                    }
                }
                else if (*cursor_p == 'S')
                {
                    uint8_t value;
                    if (parse_hex_byte(cursor_p + 1, &value))
                    {
                        dimmer_switch_type_dpid[dimmer_idx] = value;
                        cursor_p += 3;
                        continue;
                    }
                }
                else if (*cursor_p == 'M')
                {
                    uint8_t value;
                    if (parse_hex_byte(cursor_p + 1, &value))
                    {
                        dimmer_min_level_dpid[dimmer_idx] = value;
                        cursor_p += 3;
                        continue;
                    }
                }
                else if (*cursor_p == 'X')
                {
                    uint8_t value;
                    if (parse_hex_byte(cursor_p + 1, &value))
                    {
                        dimmer_max_level_dpid[dimmer_idx] = value;
                        cursor_p += 3;
                        continue;
                    }
                }
                cursor_p++;
            }
        }
        else if (entry[0] == 'X')
        {
            hal_gpio_pin_t open_pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_pin_t close_pin = hal_gpio_parse_pin(entry + 3);
            hal_gpio_pull_t pull = hal_gpio_parse_pull(entry + 5);

            hal_gpio_init(open_pin, 1, pull);
            hal_gpio_init(close_pin, 1, pull);

            buttons[buttons_cnt].pin = open_pin;
            buttons[buttons_cnt].long_press_duration_ms = 800;
            buttons[buttons_cnt].multi_press_duration_ms = 800;
            buttons[buttons_cnt].debounce_delay_ms = debounce_ms;
            buttons[buttons_cnt].on_multi_press = on_multi_press_reset;
            button_t *open_button = &buttons[buttons_cnt++];

            buttons[buttons_cnt].pin = close_pin;
            buttons[buttons_cnt].long_press_duration_ms = 800;
            buttons[buttons_cnt].multi_press_duration_ms = 800;
            buttons[buttons_cnt].debounce_delay_ms = debounce_ms;
            buttons[buttons_cnt].on_multi_press = on_multi_press_reset;
            button_t *close_button = &buttons[buttons_cnt++];

            cover_switch_clusters[cover_switch_clusters_cnt].open_button =
                open_button;
            cover_switch_clusters[cover_switch_clusters_cnt].close_button =
                close_button;
            cover_switch_clusters[cover_switch_clusters_cnt].cover_switch_idx =
                cover_switch_clusters_cnt;
            cover_switch_clusters_cnt++;
        }
        else if (entry[0] == 'C')
        {
            hal_gpio_pin_t open_pin = hal_gpio_parse_pin(entry + 1);
            hal_gpio_pin_t close_pin = hal_gpio_parse_pin(entry + 3);

            hal_gpio_init(open_pin, 0, HAL_GPIO_PULL_NONE);
            hal_gpio_init(close_pin, 0, HAL_GPIO_PULL_NONE);

            relays[relays_cnt].pin = open_pin;
            relays[relays_cnt].on_high = 1;
            relays[relays_cnt].is_latching = 0;
            relay_t *open_relay = &relays[relays_cnt++];

            relays[relays_cnt].pin = close_pin;
            relays[relays_cnt].on_high = 1;
            relays[relays_cnt].is_latching = 0;
            relay_t *close_relay = &relays[relays_cnt++];

            cover_clusters[cover_clusters_cnt].open_relay = open_relay;
            cover_clusters[cover_clusters_cnt].close_relay = close_relay;
            cover_clusters[cover_clusters_cnt].cover_idx = cover_clusters_cnt;
            cover_clusters_cnt++;
        }
        else if (entry[0] == 'i')
        {
            uint32_t image_type = parse_int(entry + 1);
            hal_zigbee_set_image_type(image_type);
        }
        else if (entry[0] == 'M')
        {
            for (int index = 0; index < switch_clusters_cnt; index++)
            {
                switch_clusters[index].mode =
                    ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY;
            }
        }
    }

    // Resolve dimmer DPIDs now that the whole config string has been parsed,
    // since Pxx tokens may appear before or after the DM<N> token.
    for (int i = 0; i < dimmer_clusters_cnt; i++)
    {
        dimmer_clusters[i].onoff_dpid = dimmer_onoff_dpid[i];
        dimmer_clusters[i].level_dpid = dimmer_level_dpid[i];
        dimmer_clusters[i].switch_type_dpid = dimmer_switch_type_dpid[i];
        dimmer_clusters[i].min_level_dpid = dimmer_min_level_dpid[i];
        dimmer_clusters[i].max_level_dpid = dimmer_max_level_dpid[i];
        dimmer_clusters[i].power_on_behavior_dpid = dimmer_power_on_behavior_dpid;
    }

    peripherals_init();

    printf("Initializing Zigbee with %d switches, %d relays, %d dimmers, %d cover switches, "
           "%d covers\r\n",
           switch_clusters_cnt, relay_clusters_cnt, dimmer_clusters_cnt, cover_switch_clusters_cnt,
           cover_clusters_cnt);

    uint8_t total_endpoints = switch_clusters_cnt + relay_clusters_cnt + dimmer_clusters_cnt +
                              cover_switch_clusters_cnt + cover_clusters_cnt;

    hal_zigbee_cluster *cluster_ptr = clusters;

    for (int index = 0; index < switch_clusters_cnt; index++)
    {
        if (switch_clusters[index].relay_index > relay_clusters_cnt)
        {
            // Detach switches that point past the available relay count.
            switch_clusters[index].relay_mode =
                ZCL_ONOFF_CONFIGURATION_RELAY_MODE_DETACHED;
            switch_clusters[index].relay_index = 0;
        }
    }

    // special case when no switches or relays are defined, so we can init a
    // "clean" device and configure it while running endpoint 1 still needs to be
    // initialised even though wenn no switches or relays are defined, so it can
    // join the network!
    if (total_endpoints == 0)
        total_endpoints = 1;

    for (int index = 0; index < total_endpoints; index++)
    {
        endpoints[index].endpoint = index + 1;
        endpoints[index].profile_id = 0x0104;
        endpoints[index].device_id = 0xffff;
    }

    endpoints[0].clusters = cluster_ptr;
    basic_cluster_add_to_endpoint(&basic_cluster, &endpoints[0]);

    hal_ota_cluster_setup(&endpoints[0].clusters[endpoints[0].cluster_count]);
    endpoints[0].cluster_count++;

    /* Lets the coordinator hand us a clock. The secondary MCU asks for
       the time and misbehaves without it; we have no RTC and no way to
       ask, so we accept a write instead. */
    static zigbee_time_cluster time_cluster;
    time_cluster_add_to_endpoint(&time_cluster, &endpoints[0]);

    // Add battery cluster for battery-powered devices
    if (battery.pin != HAL_INVALID_PIN)
    {
        static zigbee_battery_cluster battery_cluster;
        battery_cluster_add_to_endpoint(&battery_cluster, &endpoints[0]);
    }

#ifdef END_DEVICE
    // Add poll control cluster for end devices
    static zigbee_poll_control_cluster poll_ctrl_cluster;
    poll_control_cluster_add_to_endpoint(&poll_ctrl_cluster, &endpoints[0],
                                         battery.pin != HAL_INVALID_PIN);
#endif

    for (int index = 0; index < switch_clusters_cnt; index++)
    {
        if (index != 0)
        {
            cluster_ptr += endpoints[index - 1].cluster_count;
            endpoints[index].clusters = cluster_ptr;
        }
        switch_cluster_add_to_endpoint(&switch_clusters[index], &endpoints[index]);
    }
    for (int index = 0; index < relay_clusters_cnt; index++)
    {
        if (switch_clusters_cnt + index != 0)
        {
            cluster_ptr += endpoints[switch_clusters_cnt + index - 1].cluster_count;
            endpoints[switch_clusters_cnt + index].clusters = cluster_ptr;
        }
        relay_cluster_add_to_endpoint(&relay_clusters[index],
                                      &endpoints[switch_clusters_cnt + index]);
        // Group cluster is stateless, safe to add to multiple endpoints
        group_cluster_add_to_endpoint(&group_cluster,
                                      &endpoints[switch_clusters_cnt + index]);
    }

    int dimmer_base = switch_clusters_cnt + relay_clusters_cnt;
    for (int index = 0; index < dimmer_clusters_cnt; index++)
    {
        if (dimmer_base + index != 0)
        {
            cluster_ptr += endpoints[dimmer_base + index - 1].cluster_count;
            endpoints[dimmer_base + index].clusters = cluster_ptr;
        }
        dimmer_cluster_add_to_endpoint(&dimmer_clusters[index],
                                       &endpoints[dimmer_base + index]);
    }

    int cover_switch_base = switch_clusters_cnt + relay_clusters_cnt;
    for (int index = 0; index < cover_switch_clusters_cnt; index++)
    {
        if (cover_switch_base + index != 0)
        {
            cluster_ptr += endpoints[cover_switch_base + index - 1].cluster_count;
            endpoints[cover_switch_base + index].clusters = cluster_ptr;
        }
        cover_switch_cluster_add_to_endpoint(&cover_switch_clusters[index],
                                             &endpoints[cover_switch_base + index]);
    }

    int cover_base =
        switch_clusters_cnt + relay_clusters_cnt + cover_switch_clusters_cnt;
    for (int index = 0; index < cover_clusters_cnt; index++)
    {
        if (cover_base + index != 0)
        {
            cluster_ptr += endpoints[cover_base + index - 1].cluster_count;
            endpoints[cover_base + index].clusters = cluster_ptr;
        }
        cover_cluster_add_to_endpoint(&cover_clusters[index],
                                      &endpoints[cover_base + index]);
    }

    hal_zigbee_init(endpoints, total_endpoints);
    while (cursor != (char *)device_config_str.data)
    {
        cursor--;
        if (*cursor == '\0')
        {
            *cursor = ';';
        }
    }

    printf("Config parsed successfully\r\n");
}

void network_indicator_on_network_status_change(
    hal_zigbee_network_status_t new_status)
{
    printf("Network status changed to %d\r\n", new_status);
    if (new_status == HAL_ZIGBEE_NETWORK_JOINED)
    {
        if (battery.pin != HAL_INVALID_PIN)
        {
            network_indicator.manual_state_when_connected = 0;
        }
        network_indicator_connected(&network_indicator);
        update_switch_clusters();
        update_relay_clusters();
    }
    else
    {
        network_indicator_not_connected(&network_indicator);
    }
}

void peripherals_init()
{
    for (int index = 0; index < buttons_cnt; index++)
    {
        btn_init(&buttons[index]);
    }
    for (int index = 0; index < leds_cnt; index++)
    {
        led_init(&leds[index]);
    }
    for (int index = 0; index < relays_cnt; index++)
    {
        relay_init(&relays[index]);
    }
    if (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED)
    {
        network_indicator_connected(&network_indicator);
        update_switch_clusters();
        update_relay_clusters();
    }
    else
    {
        network_indicator_not_connected(&network_indicator);
    }
    hal_register_on_network_status_change_callback(
        network_indicator_on_network_status_change);
}

// Helper functions

char *seek_until(char *cursor, char needle)
{
    while (*cursor != needle && *cursor != '\0')
    {
        cursor++;
    }
    return (cursor);
}

char *extract_next_entry(char **cursor)
{
    char *end = seek_until(*cursor, ';');

    *end = '\0';
    char *res = *cursor;
    *cursor = end + 1;
    return (res);
}

uint32_t parse_int(const char *s)
{
    if (!s)
        return 0;

    uint32_t n = 0;
    while (*s >= '0' && *s <= '9')
    {
        n = n * 10 + (uint32_t)(*s - '0');
        s++;
    }
    return n;
}

uint8_t parse_hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
        return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F')
        return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f')
        return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}

bool parse_hex_byte(const char *s, uint8_t *out)
{
    if (!s || !out || s[0] == '\0' || s[1] == '\0')
    {
        return false;
    }

    uint8_t hi = parse_hex_nibble(s[0]);
    uint8_t lo = parse_hex_nibble(s[1]);
    if (hi == 0xFF || lo == 0xFF)
    {
        return false;
    }
    *out = (uint8_t)((hi << 4) | lo);
    return true;
}
