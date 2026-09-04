#include "basic_cluster.h"
#include "consts.h"
#include "cover_cluster.h"
#include "cover_switch_cluster.h"
#include "dimmer_cluster.h"
#include "hal/printf_selector.h"
#include "poll_control_cluster.h"
#include "relay_cluster.h"
#include "switch_cluster.h"
#include "time_cluster.h"

static void zigbee_on_attr_change(uint8_t endpoint, uint16_t cluster_id,
                                  uint16_t attribute_id)
{
    printf("Attribute changed, ep: %d, cluster: %d, attr: %d\r\n", endpoint,
           cluster_id, attribute_id);
    if (cluster_id == ZCL_CLUSTER_BASIC)
    {
        basic_cluster_callback_attr_write_trampoline(attribute_id);
    }
    else if (cluster_id == ZCL_CLUSTER_ON_OFF_SWITCH_CONFIG)
    {
        // genOnOffSwitchCfg is shared by switches and dimmers; each trampoline
        // no-ops on endpoints it does not own.
        switch_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
        dimmer_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
    }
    else if (cluster_id == ZCL_CLUSTER_LIGHTING_BALLAST_CONFIG)
    {
        dimmer_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
    }
    else if (cluster_id == ZCL_CLUSTER_COVER_SWITCH_CONFIG)
    {
        cover_switch_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
    }
    else if (cluster_id == ZCL_CLUSTER_ON_OFF)
    {
        // OnOff is shared by relays and dimmers; each trampoline no-ops on
        // endpoints it does not own (StartUpOnOff is writable on both).
        relay_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
        dimmer_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
    }
    else if (cluster_id == ZCL_CLUSTER_TIME)
    {
        time_cluster_on_write_attr(attribute_id);
    }
    else if (cluster_id == ZCL_CLUSTER_WINDOW_COVERING)
    {
        cover_cluster_callback_attr_write_trampoline(endpoint, attribute_id);
    }
#ifdef END_DEVICE
    else if (cluster_id == ZCL_CLUSTER_POLL_CONTROL)
    {
        poll_control_cluster_callback_attr_write(attribute_id);
    }
#endif
}

void init_global_attr_write_callback()
{
    hal_zigbee_register_on_attribute_change_callback(zigbee_on_attr_change);
}
