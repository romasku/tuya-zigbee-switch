#include "device_config/config_parser.h"
#include "device_config/device_type.h"
#include "device_config/nvm_items.h"
#include "device_config/reset.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/system.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"
#include "zigbee/tuya_secondary_mcu.h"
#include "zigbee/time_cluster.h"
#include "zigbee/tuya_dp_relay.h"
#include "zigbee/dp_attr.h"
#include "hal/uart.h"
#include "zigbee/battery_cluster.h"
#include "zigbee/general_commands.h"

extern hal_uart_config_t mcu_uart_config;
#ifdef END_DEVICE
#include "zigbee/poll_control_cluster.h"

#endif

void process_device_type_change()
{
    // If device was updated from router to end device or vice versa,
    // we need to do a reset, as the network settings stored by SDK in NVM
    // are not compatible between these device types.
    // Read device type from NVM and compare with current configuration.
    enum device_type_t stored_device_type;
    hal_nvm_status_t st =
        hal_nvm_read(NV_ITEM_DEVICE_TYPE, sizeof(stored_device_type),
                     (uint8_t *)&stored_device_type);

    if (st != HAL_NVM_SUCCESS)
    {
        // Unable to read device type from NVM, possibly first boot.
        stored_device_type = CURRENT_DEVICE_TYPE;
        hal_nvm_write(NV_ITEM_DEVICE_TYPE, sizeof(stored_device_type),
                      (uint8_t *)&stored_device_type);
        return;
    }
    if (stored_device_type != CURRENT_DEVICE_TYPE)
    {
        printf("Device type change detected: %d -> %d\r\n", stored_device_type,
               CURRENT_DEVICE_TYPE);
        // Device type has changed, update NVM and reset device.
        stored_device_type = CURRENT_DEVICE_TYPE;
        hal_nvm_write(NV_ITEM_DEVICE_TYPE, sizeof(stored_device_type),
                      (uint8_t *)&stored_device_type);
        // Perform a factory reset to clear incompatible network settings.
        hal_factory_reset();
        schedule_reboot(2000);
    }
}

// 0x03 = reset/pair module, sent MCU->module when the dimmer's physical
// button is held down. Data 0x01 means "leave current network and join a new
// one" -> a pairing reset (not a factory reset of the user's config).
#define TUYA_MCU_RESET_PAIR_NETWORK 0x03
#define TUYA_MCU_RESET_PAIR_REJOIN 0x01
/* Queries the MCU sends to the module (Tuya Zigbee UART protocol). */
#define TUYA_MCU_REPORT_NETWORK_STATUS 0x02
#define TUYA_MCU_QUERY_NETWORK_STATUS 0x20
#define TUYA_MCU_SYNC_TIME            0x24
#define TUYA_MCU_QUERY_GATEWAY_STATUS 0x25
#define TUYA_NET_STATUS_NOT_CONNECTED 0x00
#define TUYA_NET_STATUS_CONNECTED     0x01
#define TUYA_GW_STATUS_OFFLINE        0x00
#define TUYA_GW_STATUS_ONLINE         0x01

/* Set once we have asked the MCU for a full datapoint dump. */
static uint8_t dp_query_sent = 0;

static void tuya_secondary_mcu_on_command(uint8_t cmd, uint16_t seq,
                                          const uint8_t *data,
                                          uint16_t data_len)
{
    if (cmd == TUYA_MCU_RESET_PAIR_NETWORK &&
        data_len == 1 && data[0] == TUYA_MCU_RESET_PAIR_REJOIN)
    {
        /* Holding a key makes the MCU ask us to leave and re-pair. An
           accidental long press is the single most common way a switch
           drops off a production network, so we refuse: a working network
           is never abandoned on a physical gesture. Tell the MCU we are
           connected instead, which is what stops its pairing blink. */
        if (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED)
        {
            uint8_t status = TUYA_NET_STATUS_CONNECTED;
            printf("Ignoring MCU leave request: already joined\r\n");
            tuya_secondary_mcu_send_cmd(TUYA_MCU_REPORT_NETWORK_STATUS,
                                        tuya_secondary_mcu_next_tx_seq(),
                                        &status, 1);
            return;
        }
        /* Already off the network: nothing to lose, so make the gesture
           useful and kick off a fresh join attempt. */
        printf("MCU leave request while unjoined: restarting steering\r\n");
        hal_zigbee_start_network_steering();
        return;
    }

    /* The MCU polls the module about the network. Leaving these unanswered
       makes it assume the link is broken, which on switch hardware shows up
       as the key LEDs blinking. Responses must echo the request's seq. */
    if (cmd == TUYA_MCU_QUERY_NETWORK_STATUS)
    {
        uint8_t status =
            (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED)
                ? TUYA_NET_STATUS_CONNECTED
                : TUYA_NET_STATUS_NOT_CONNECTED;
        tuya_secondary_mcu_send_cmd(cmd, seq, &status, 1);

        /* First contact from the MCU is the earliest point we know it is
           listening, so this is where we ask it to dump every datapoint.
           Doing it from app_init() would race the MCU's own boot. */
        if (!dp_query_sent)
        {
            dp_query_sent = 1;
            /* Apply the IT-declared datapoint values here, not from
               app_init(). Writing the config string reboots only the
               Telink; the MCU keeps running mid conversation and drops
               whatever we send before it is back in sync. That is why
               changing a setting used to need a power cycle to stick. */
            tuya_dp_apply_inits();
            dp_attr_query_all();
        }
        return;
    }

    if (cmd == TUYA_MCU_QUERY_GATEWAY_STATUS)
    {
        /* We are the gateway from the MCU's point of view: if we are on a
           network, report the gateway as reachable. */
        uint8_t status =
            (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED)
                ? TUYA_GW_STATUS_ONLINE
                : TUYA_GW_STATUS_OFFLINE;
        tuya_secondary_mcu_send_cmd(cmd, seq, &status, 1);
        return;
    }

    if (cmd == TUYA_MCU_SYNC_TIME)
    {
        /* Tuya wants eight bytes: UTC then local, both seconds since the
           Unix epoch. We used to send zeros here, on the assumption the MCU
           only kept time for countdowns. That was never checked, and epoch
           1970 is the one difference that lines up with touch zones drifting
           on zigbee2mqtt but never on the Tuya gateway, which supplies real
           time. Zeros remain the answer only while the coordinator has not
           told us anything -- claiming a time we do not have would be worse.
           No timezone is applied: local equals UTC until we are given one. */
        uint8_t  t[8] = {0};
        uint32_t now_s = zigbee_time_unix();
        if (now_s != 0)
        {
            t[0] = (uint8_t)(now_s >> 24); t[1] = (uint8_t)(now_s >> 16);
            t[2] = (uint8_t)(now_s >> 8);  t[3] = (uint8_t)now_s;
            t[4] = t[0]; t[5] = t[1]; t[6] = t[2]; t[7] = t[3];
        }
        tuya_secondary_mcu_send_cmd(cmd, seq, t, sizeof(t));
        return;
    }
}

void app_init(void)
{
    handle_version_changes();
    parse_config(); // Does most of the setup, including all callbacks
                    // registration

    // Devices with dimmers OR DP-backed relays talk to a secondary MCU over
    // UART. Initialising it unconditionally would reassign the UART pins on
    // the many Telink boards that use them as GPIOs, so gate it on the config.
    if (dimmer_clusters_cnt > 0 || tuya_dp_relay_count() > 0 || dp_attrs_cnt > 0)
    {
        tuya_secondary_mcu_init(&mcu_uart_config);
        tuya_dp_relay_init();
        // Registered after tuya_dp_relay_init so it sits at the head of the
        // report callback chain and forwards what it does not own.
        dp_attr_init();
        tuya_secondary_mcu_register_command_callback(tuya_secondary_mcu_on_command);
    }

    hal_zigbee_init_ota();
    init_global_attr_write_callback();

    process_device_type_change();
}

static bool boot_announce_sent = false;

void app_task()
{
#ifdef END_DEVICE
    poll_control_cluster_update();
#endif

    if (dimmer_clusters_cnt > 0 || tuya_dp_relay_count() > 0 || dp_attrs_cnt > 0)
    {
        tuya_secondary_mcu_poll();
    }

    // TODO: add jitter to avoid all devices trying to join at once
    if (hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINED &&
        hal_zigbee_get_network_status() != HAL_ZIGBEE_NETWORK_JOINING)
    {
        hal_zigbee_start_network_steering();
    }
    if (!boot_announce_sent &&
        hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED)
    {
        hal_zigbee_send_announce();
        boot_announce_sent = true;
    }
}
