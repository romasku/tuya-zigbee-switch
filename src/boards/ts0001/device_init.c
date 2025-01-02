#include "zigbee/endpoint.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"
#include "zigbee/general.h"
#include "peripherals.h"

zigbee_endpoint main_endpoint = {
	.index = 1,	
};


zigbee_endpoint relay1_endpoint = {
	.index = 2,	
};

zigbee_basic_cluster basic_cluster = {
	.deviceEnable = 1,
};

zigbee_relay_cluster relay1_cluster = {
	.relay = &relay1,
};


zigbee_relay_cluster* relay_clusters[1] = {&relay1_cluster};


zigbee_switch_cluster switch1_cluster = {
	.mode = ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE,
	.action = ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE,
	.relay_mode = ZCL_ONOFF_CONFIGURATION_RELAY_MODE_RISE,
	.relay_index = 1,
	.button = &button_s1,
};


void init_zcl_endpoints() {
    zigbee_endpoint_init(&main_endpoint);
	zigbee_endpoint_init(&relay1_endpoint);

	basic_cluster_add_to_endpoint(&basic_cluster, &main_endpoint);
	switch_cluster_add_to_endpoint(&switch1_cluster, &main_endpoint);
	relay_cluster_add_to_endpoint(&relay1_cluster, &relay1_endpoint);

	zigbee_endpoint_add_cluster(&main_endpoint, 0, ZCL_CLUSTER_OTA);

	zigbee_endpoint_register_self(&main_endpoint);
	zigbee_endpoint_register_self(&relay1_endpoint);
    
}

void init_reporting() {
    u32 reportableChange = 1;
    bdb_defaultReportingCfg(
        relay1_endpoint.index,
        HA_PROFILE_ID,
        ZCL_CLUSTER_GEN_ON_OFF,
        ZCL_ATTRID_ONOFF,
        0,
        60,
        (u8 *)&reportableChange
    );
}