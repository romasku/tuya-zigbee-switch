#include "hal/gpio.h"
#include "hal/printf_selector.h"
#include "hal/zigbee.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/consts.h"
#include "zigbee/group_cluster.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"

#include <stdint.h>
#include <string.h>

#include "base_components/led.h"
#include "base_components/network_indicator.h"
#include "config_nv.h"
#include "device_config/reset.h"
#include "hal/system.h"
#include "hal/zigbee.h"
#include "hal/zigbee_ota.h"

// Forward declarations
void periferals_init(void);

// extern ota_preamble_t baseEndpoint_otaInfo;

network_indicator_t network_indicator = {
    .leds = {NULL, NULL, NULL, NULL},
    .has_dedicated_led = 0,
    .manual_state_when_connected = 1,
};

led_t leds[5];
uint8_t leds_cnt = 0;

button_t buttons[5];
uint8_t buttons_cnt = 0;

relay_t relays[5];
uint8_t relays_cnt = 0;

zigbee_basic_cluster basic_cluster = {
    .deviceEnable = 1,
};

zigbee_group_cluster group_cluster = {};

zigbee_switch_cluster switch_clusters[4];
uint8_t switch_clusters_cnt = 0;

zigbee_relay_cluster relay_clusters[4];
uint8_t relay_clusters_cnt = 0;

hal_zigbee_cluster clusters[32];
hal_zigbee_endpoint endpoints[10];

uint32_t parse_int(const char *s);
char *seek_until(char *cursor, char needle);
char *extract_next_entry(char **cursor);

void on_reset_clicked(void *_) { reset_all(); }

void parse_config() {
  device_config_read_from_nv();
  char *cursor = device_config_str.data;

  const char *zb_manufacturer = extract_next_entry(&cursor);

  basic_cluster.manuName[0] = strlen(zb_manufacturer);
  if (basic_cluster.manuName[0] > 31) {
    printf("Manufacturer too big\r\n");
    reset_all();
  }
  memcpy(basic_cluster.manuName + 1, zb_manufacturer,
         basic_cluster.manuName[0]);

  const char *zb_model = extract_next_entry(&cursor);
  basic_cluster.modelId[0] = strlen(zb_model);
  if (basic_cluster.modelId[0] > 31) {
    printf("Model too big\r\n");
    reset_all();
  }
  memcpy(basic_cluster.modelId + 1, zb_model, basic_cluster.modelId[0]);

  bool has_dedicated_status_led = false;
  char *entry;
  for (entry = extract_next_entry(&cursor); *entry != '\0';
       entry = extract_next_entry(&cursor)) {
    if (entry[0] == 'B') {
      hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
      hal_gpio_pull_t pull = hal_gpio_parse_pull(entry + 3);
      hal_gpio_init(pin, 1, pull);

      buttons[buttons_cnt].pin = pin;
      buttons[buttons_cnt].long_press_duration_ms = 2000;
      buttons[buttons_cnt].multi_press_duration_ms = 800;
      buttons[buttons_cnt].on_long_press = on_reset_clicked;
      buttons_cnt++;
    } else if (entry[0] == 'L') {
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
    } else if (entry[0] == 'I') {
      hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
      hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);
      leds[leds_cnt].pin = pin;
      leds[leds_cnt].on_high = entry[3] != 'i';
      led_init(&leds[leds_cnt]);

      for (int index = 0; index < 4; index++) {
        if (relay_clusters[index].indicator_led == NULL) {
          relay_clusters[index].indicator_led = &leds[leds_cnt];
          break;
        }
      }

      if (!has_dedicated_status_led) {
        for (int index = 0; index < 4; index++) {
          if (network_indicator.leds[index] == NULL) {
            network_indicator.leds[index] = &leds[leds_cnt];
            break;
          }
        }
      }
      leds_cnt++;
    } else if (entry[0] == 'S') {
      hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
      hal_gpio_pull_t pull = hal_gpio_parse_pull(entry + 3);
      hal_gpio_init(pin, 1, pull);

      buttons[buttons_cnt].pin = pin;
      buttons[buttons_cnt].long_press_duration_ms = 800;
      buttons[buttons_cnt].multi_press_duration_ms = 800;

      switch_clusters[switch_clusters_cnt].switch_idx = switch_clusters_cnt;
      switch_clusters[switch_clusters_cnt].mode =
          ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE;
      switch_clusters[switch_clusters_cnt].action =
          ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE;
      switch_clusters[switch_clusters_cnt].relay_mode =
          ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT;
      switch_clusters[switch_clusters_cnt].binded_mode =
          ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT;
      switch_clusters[switch_clusters_cnt].relay_index =
          switch_clusters_cnt + 1;
      switch_clusters[switch_clusters_cnt].button = &buttons[buttons_cnt];
      switch_clusters[switch_clusters_cnt].level_move_rate = 50;
      buttons_cnt++;
      switch_clusters_cnt++;
    } else if (entry[0] == 'R') {
      hal_gpio_pin_t pin = hal_gpio_parse_pin(entry + 1);
      hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);

      relays[relays_cnt].pin = pin;
      relays[relays_cnt].on_high = 1;

      if (entry[3] != '\0') {
        pin = hal_gpio_parse_pin(entry + 3);
        hal_gpio_init(pin, 0, HAL_GPIO_PULL_NONE);
        relays[relays_cnt].off_pin = pin;
      }

      relay_clusters[relay_clusters_cnt].relay_idx = relay_clusters_cnt;
      relay_clusters[relay_clusters_cnt].relay = &relays[relays_cnt];

      relays_cnt++;
      relay_clusters_cnt++;
    } else if (entry[0] == 'i') {
      //      u32 image_type = parseInt(entry + 1);
      //      baseEndpoint_otaInfo.imageType = image_type;
    } else if (entry[0] == 'M') {
      for (int index = 0; index < switch_clusters_cnt; index++) {
        switch_clusters[index].mode =
            ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY;
      }
    }
  }

  periferals_init();

  uint8_t total_endpoints = switch_clusters_cnt + relay_clusters_cnt;

  hal_zigbee_cluster *cluster_ptr = clusters;

  // special case when no switches or relays are defined, so we can init a
  // "clean" device and configure it while running endpoint 1 still needs to be
  // initialised even though wenn no switches or relays are defined, so it can
  // join the network!
  if (total_endpoints == 0)
    total_endpoints = 1;

  for (int index = 0; index < total_endpoints; index++) {
    endpoints[index].endpoint = index + 1;
    endpoints[index].profile_id = 0x0104;
    endpoints[index].device_id = 0xffff;
  }

  endpoints[0].clusters = cluster_ptr;
  basic_cluster_add_to_endpoint(&basic_cluster, &endpoints[0]);

  hal_ota_cluster_setup(&endpoints[0].clusters[endpoints[0].cluster_count]);
  endpoints[0].cluster_count++;

  for (int index = 0; index < switch_clusters_cnt; index++) {
    if (index != 0) {
      cluster_ptr += endpoints[index - 1].cluster_count;
      endpoints[index].clusters = cluster_ptr;
    }
    switch_cluster_add_to_endpoint(&switch_clusters[index], &endpoints[index]);
  }
  for (int index = 0; index < relay_clusters_cnt; index++) {
    cluster_ptr += endpoints[switch_clusters_cnt + index - 1].cluster_count;
    endpoints[switch_clusters_cnt + index].clusters = cluster_ptr;
    relay_cluster_add_to_endpoint(&relay_clusters[index],
                                  &endpoints[switch_clusters_cnt + index]);
    // Group cluster is stateless, safe to add to multiple endpoints
    group_cluster_add_to_endpoint(&group_cluster,
                                  &endpoints[switch_clusters_cnt + index]);
  }

  hal_zigbee_init(endpoints, total_endpoints);
  while (cursor != (char *)device_config_str.data) {
    cursor--;
    if (*cursor == '\0') {
      *cursor = ';';
    }
  }
}

void network_indicator_on_network_status_change(
    hal_zigbee_network_status_t new_status) {
  if (new_status == HAL_ZIGBEE_NETWORK_JOINED) {
    network_indicator_connected(&network_indicator);
  } else {
    network_indicator_not_connected(&network_indicator);
  }
}

void periferals_init() {
  for (int index = 0; index < buttons_cnt; index++) {
    btn_init(&buttons[index]);
  }
  if (hal_zigbee_get_network_status() == HAL_ZIGBEE_NETWORK_JOINED) {
    network_indicator_connected(&network_indicator);
  } else {
    network_indicator_not_connected(&network_indicator);
  }
  hal_register_on_network_status_change_callback(
      network_indicator_on_network_status_change);
}

void init_reporting() {
  //  u32 reportableChange = 1;
  //
  //  for (int index = 0; index < relay_clusters_cnt; index++)
  //  {
  //    bdb_defaultReportingCfg(
  //      switch_clusters_cnt + index + 1,
  //      HA_PROFILE_ID,
  //      ZCL_CLUSTER_GEN_ON_OFF,
  //      ZCL_ATTRID_ONOFF,
  //      0,
  //      60,
  //      (u8 *)&reportableChange
  //      );
  //  }
}

// Helper functions

char *seek_until(char *cursor, char needle) {
  while (*cursor != needle && *cursor != '\0') {
    cursor++;
  }
  return (cursor);
}

char *extract_next_entry(char **cursor) {
  char *end = seek_until(*cursor, ';');

  *end = '\0';
  char *res = *cursor;
  *cursor = end + 1;
  return (res);
}

uint32_t parse_int(const char *s) {
  if (!s)
    return 0;

  uint32_t n = 0;
  while (*s >= '0' && *s <= '9') {
    n = n * 10 + (uint32_t)(*s - '0');
    s++;
  }
  return n;
}
