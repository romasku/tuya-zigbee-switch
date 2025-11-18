#include "zigbee/endpoint.h"
#include "zigbee/basic_cluster.h"
#include "zigbee/group_cluster.h"
#include "zigbee/relay_cluster.h"
#include "zigbee/switch_cluster.h"
#include "zigbee/scene_cluster.h"
#include "zigbee/general.h"
#include "ota.h"

#include "base_components/led.h"
#include "base_components/network_indicator.h"
#include "chip_8258/gpio.h"
#include "config_nv.h"

extern ota_preamble_t baseEndpoint_otaInfo;

network_indicator_t network_indicator =
{
  .leds                  = { NULL, NULL, NULL, NULL },
  .has_dedicated_led = 0,
  .manual_state_when_connected = 1,
};

led_t leds[5];
u8    leds_cnt = 0;

button_t buttons[5];
u8       buttons_cnt = 0;

relay_t relays[5];
u8      relays_cnt = 0;

zigbee_basic_cluster basic_cluster =
{
  .deviceEnable = 1,
};

zigbee_group_cluster group_cluster = {};

move_t level_move[4];
zigbee_switch_cluster switch_clusters[4];
u8 switch_clusters_cnt = 0;

zigbee_relay_cluster relay_clusters[4];
u8 relay_clusters_cnt = 0;

// Each relay cluster should have one scene cluster
zigbee_scene_cluster scene_clusters[4];
#define scene_clusters_cnt relay_clusters_cnt

zigbee_endpoint endpoints[10];


void reset_to_default_config();
GPIO_PinTypeDef parsePin(const char *pin_str);
GPIO_PullTypeDef parsePullUpDown(const char *pull_str);
u32 parseInt(const char *s);
char *seekUntil(char *cursor, char needle);
char *extractNextEntry(char **cursor);
void init_gpio_input(GPIO_PinTypeDef pin, GPIO_PullTypeDef pull);
void init_gpio_output(GPIO_PinTypeDef pin);



void onResetClicked(void *_)
{
  factoryReset();
}

void parse_config()
{
  device_config_read_from_nv();
  char *cursor = device_config_str.data;

  const char *zb_manufacturer = extractNextEntry(&cursor);

  basic_cluster.manuName[0] = strlen(zb_manufacturer);
  if (basic_cluster.manuName[0] > 31)
  {
    printf("Manufacturer too big\r\n");
    reset_to_default_config();
  }
  memcpy(basic_cluster.manuName + 1, zb_manufacturer, basic_cluster.manuName[0]);

  const char *zb_model = extractNextEntry(&cursor);
  basic_cluster.modelId[0] = strlen(zb_model);
  if (basic_cluster.modelId[0] > 31)
  {
    printf("Model too big\r\n");
    reset_to_default_config();
  }
  memcpy(basic_cluster.modelId + 1, zb_model, basic_cluster.modelId[0]);

  bool  has_dedicated_status_led = false;
  char *entry;
  for (entry = extractNextEntry(&cursor); *entry != '\0'; entry = extractNextEntry(&cursor))
  {
    if (entry[0] == 'B')
    {
      GPIO_PinTypeDef  pin  = parsePin(entry + 1);
      GPIO_PullTypeDef pull = parsePullUpDown(entry + 3);
      init_gpio_input(pin, pull);

      buttons[buttons_cnt].pin = pin;
      buttons[buttons_cnt].long_press_duration_ms  = 2000;
      buttons[buttons_cnt].multi_press_duration_ms = 800;
      buttons[buttons_cnt].on_long_press           = onResetClicked;
      buttons_cnt++;
    }
    else if (entry[0] == 'L')
    {
      GPIO_PinTypeDef pin = parsePin(entry + 1);
      init_gpio_output(pin);
      leds[leds_cnt].pin     = pin;
      leds[leds_cnt].on_high = entry[3] != 'i';

      led_init(&leds[leds_cnt]);

      network_indicator.leds[0] = &leds[leds_cnt];
      network_indicator.leds[1] = NULL;
      network_indicator.has_dedicated_led = true;

      has_dedicated_status_led = true;
      leds_cnt++;
    }
    else if (entry[0] == 'I')
    {
      GPIO_PinTypeDef pin = parsePin(entry + 1);
      init_gpio_output(pin);
      leds[leds_cnt].pin     = pin;
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
    else if (entry[0] == 'S')
    {
      GPIO_PinTypeDef  pin  = parsePin(entry + 1);
      GPIO_PullTypeDef pull = parsePullUpDown(entry + 3);
      init_gpio_input(pin, pull);

      buttons[buttons_cnt].pin = pin;
      buttons[buttons_cnt].long_press_duration_ms  = 800;
      buttons[buttons_cnt].multi_press_duration_ms = 800;

      level_move[switch_clusters_cnt].moveMode = LEVEL_MOVE_UP,
      level_move[switch_clusters_cnt].rate = 50,
      level_move[switch_clusters_cnt].optPresent = 0,
      level_move[switch_clusters_cnt].optionsMask = 0,
      level_move[switch_clusters_cnt].optionsOverride = 0,

      switch_clusters[switch_clusters_cnt].switch_idx  = switch_clusters_cnt;
      switch_clusters[switch_clusters_cnt].mode        = ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_TOGGLE;
      switch_clusters[switch_clusters_cnt].action      = ZCL_ONOFF_CONFIGURATION_SWITCH_ACTION_TOGGLE_SIMPLE;
      switch_clusters[switch_clusters_cnt].relay_mode  = ZCL_ONOFF_CONFIGURATION_RELAY_MODE_SHORT;
      switch_clusters[switch_clusters_cnt].binded_mode = ZCL_ONOFF_CONFIGURATION_BINDED_MODE_SHORT;
      switch_clusters[switch_clusters_cnt].relay_index = switch_clusters_cnt + 1;
      switch_clusters[switch_clusters_cnt].button      = &buttons[buttons_cnt];
      switch_clusters[switch_clusters_cnt].level_move  = &level_move[switch_clusters_cnt];

      buttons_cnt++;
      switch_clusters_cnt++;
    }
    else if (entry[0] == 'R')
    {
      GPIO_PinTypeDef pin = parsePin(entry + 1);
      init_gpio_output(pin);

      relays[relays_cnt].pin     = pin;
      relays[relays_cnt].on_high = 1;

      if (entry[3] != '\0')
      {
        pin = parsePin(entry + 3);
        init_gpio_output(pin);
        relays[relays_cnt].off_pin = pin;
      }

      relay_clusters[relay_clusters_cnt].relay_idx = relay_clusters_cnt;
      relay_clusters[relay_clusters_cnt].relay = &relays[relays_cnt];

      relays_cnt++;
      relay_clusters_cnt++;
    }
    else if (entry[0] == 'i')
    {
      u32 image_type = parseInt(entry + 1);
      baseEndpoint_otaInfo.imageType = image_type;
    }
    else if (entry[0] == 'M')
    {
      for (int index = 0; index < switch_clusters_cnt; index++)
      {
        switch_clusters[index].mode = ZCL_ONOFF_CONFIGURATION_SWITCH_TYPE_MOMENTARY;
      }
    }
  }

  periferals_init();

  u8 total_endpoints = switch_clusters_cnt + relay_clusters_cnt;

  // special case when no switches or relays are defined, so we can init a "clean" device and configure it while running
  // endpoint 1 still needs to be initialised even though wenn no switches or relays are defined, so it can join the network!
  if (total_endpoints == 0) total_endpoints = 1;

  for (int index = 0; index < total_endpoints; index++)
  {
    endpoints[index].index = index + 1;
  }

  basic_cluster_populate(&basic_cluster);
  zigbee_endpoint_add_cluster(&endpoints[0], 0, ZCL_CLUSTER_OTA);

  for (int index = 0; index < switch_clusters_cnt; index++)
  {
    zigbee_endpoint_init(&endpoints[index], HA_DEV_ONOFF_SWITCH);
    switch_cluster_add_to_endpoint(&switch_clusters[index], &endpoints[index]);
  }
  for (int index = 0; index < relay_clusters_cnt; index++)
  {
    relay_clusters[index].scene_cluster = scene_clusters + index;
    scene_clusters[index].relay_cluster = relay_clusters + index;

    zigbee_endpoint_init(&endpoints[index], HA_DEV_ONOFF_OUTPUT);

    relay_cluster_add_to_endpoint(&relay_clusters[index], &endpoints[switch_clusters_cnt + index]);
    // Group cluster is stateless, safe to add to multiple endpoints
    group_cluster_add_to_endpoint(&group_cluster, &endpoints[switch_clusters_cnt + index]);

    scene_cluster_add_to_endpoint(scene_clusters + index, &endpoints[switch_clusters_cnt + index]);
  }

  for (int index = 0; index < total_endpoints; index++)
  {
    basic_cluster_add_to_endpoint(&basic_cluster, &endpoints[index]);
    zigbee_endpoint_register_self(&endpoints[index]);
  }
  cursor--;
  while (cursor != device_config_str.data)
  {
    cursor--;
    if (*cursor == '\0')
    {
      *cursor = ';';
    }
  }
}


void periferals_init()
{
  millis_init();
  for (int index = 0; index < buttons_cnt; index++)
  {
    btn_init(&buttons[index]);
  }
}


void periferals_update()
{
  for (int index = 0; index < leds_cnt; index++)
  {
    led_update(&leds[index]);
  }
  for (int index = 0; index < buttons_cnt; index++)
  {
    btn_update(&buttons[index]);
  }
  for (int index = 0; index < relays_cnt; ++index)
  {
    relay_poll(&relays[index]);
  }
}

void init_reporting()
{
  u32 reportableChange = 1;

  for (int index = 0; index < relay_clusters_cnt; index++)
  {
    bdb_defaultReportingCfg(
      switch_clusters_cnt + index + 1,
      HA_PROFILE_ID,
      ZCL_CLUSTER_GEN_ON_OFF,
      ZCL_ATTRID_ONOFF,
      0,
      60,
      (u8 *)&reportableChange
      );
  }
}

// Helper functions

void init_gpio_input(GPIO_PinTypeDef pin, GPIO_PullTypeDef pull)
{
  gpio_set_func(pin, AS_GPIO);
  gpio_set_input_en(pin, 1);
  gpio_set_output_en(pin, 0);
  gpio_setup_up_down_resistor(pin, pull);
}

void init_gpio_output(GPIO_PinTypeDef pin)
{
  gpio_set_func(pin, AS_GPIO);
  gpio_set_input_en(pin, 0);
  gpio_set_output_en(pin, 1);
}

__attribute__((noreturn)) void reset_to_default_config()
{
  printf("RESET reset_to_default_config\r\n");
  device_config_remove_from_nv();
  zb_resetDevice();
  while (1)
  {
    ;
  }
}

GPIO_PinTypeDef parsePin(const char *pin_str)
{
  if (pin_str[0] == 'A')
  {
    if (pin_str[1] == '0')
    {
      return(GPIO_PA0);
    }
    if (pin_str[1] == '1')
    {
      return(GPIO_PA1);
    }
    if (pin_str[1] == '2')
    {
      return(GPIO_PA2);
    }
    if (pin_str[1] == '3')
    {
      return(GPIO_PA3);
    }
    if (pin_str[1] == '4')
    {
      return(GPIO_PA4);
    }
    if (pin_str[1] == '5')
    {
      return(GPIO_PA5);
    }
    if (pin_str[1] == '6')
    {
      return(GPIO_PA6);
    }
    if (pin_str[1] == '7')
    {
      return(GPIO_PA7);
    }
  }
  if (pin_str[0] == 'B')
  {
    if (pin_str[1] == '0')
    {
      return(GPIO_PB0);
    }
    if (pin_str[1] == '1')
    {
      return(GPIO_PB1);
    }
    if (pin_str[1] == '2')
    {
      return(GPIO_PB2);
    }
    if (pin_str[1] == '3')
    {
      return(GPIO_PB3);
    }
    if (pin_str[1] == '4')
    {
      return(GPIO_PB4);
    }
    if (pin_str[1] == '5')
    {
      return(GPIO_PB5);
    }
    if (pin_str[1] == '6')
    {
      return(GPIO_PB6);
    }
    if (pin_str[1] == '7')
    {
      return(GPIO_PB7);
    }
  }
  if (pin_str[0] == 'C')
  {
    if (pin_str[1] == '0')
    {
      return(GPIO_PC0);
    }
    if (pin_str[1] == '1')
    {
      return(GPIO_PC1);
    }
    if (pin_str[1] == '2')
    {
      return(GPIO_PC2);
    }
    if (pin_str[1] == '3')
    {
      return(GPIO_PC3);
    }
    if (pin_str[1] == '4')
    {
      return(GPIO_PC4);
    }
    if (pin_str[1] == '5')
    {
      return(GPIO_PC5);
    }
    if (pin_str[1] == '6')
    {
      return(GPIO_PC6);
    }
    if (pin_str[1] == '7')
    {
      return(GPIO_PC7);
    }
  }
  if (pin_str[0] == 'D')
  {
    if (pin_str[1] == '0')
    {
      return(GPIO_PD0);
    }
    if (pin_str[1] == '1')
    {
      return(GPIO_PD1);
    }
    if (pin_str[1] == '2')
    {
      return(GPIO_PD2);
    }
    if (pin_str[1] == '3')
    {
      return(GPIO_PD3);
    }
    if (pin_str[1] == '4')
    {
      return(GPIO_PD4);
    }
    if (pin_str[1] == '5')
    {
      return(GPIO_PD5);
    }
    if (pin_str[1] == '6')
    {
      return(GPIO_PD6);
    }
    if (pin_str[1] == '7')
    {
      return(GPIO_PD7);
    }
  }
  if (pin_str[0] == 'E')
  {
    if (pin_str[1] == '0')
    {
      return(GPIO_PE0);
    }
    if (pin_str[1] == '1')
    {
      return(GPIO_PE1);
    }
    if (pin_str[1] == '2')
    {
      return(GPIO_PE2);
    }
    if (pin_str[1] == '3')
    {
      return(GPIO_PE3);
    }
  }
  printf("Failed to parse ping: %s\r\n", pin_str);
  reset_to_default_config();
}

GPIO_PullTypeDef parsePullUpDown(const char *pull_str)
{
  if (pull_str[0] == 'u')
  {
    return(PM_PIN_PULLUP_10K);
  }
  if (pull_str[0] == 'U')
  {
    return(PM_PIN_PULLUP_1M);
  }
  if (pull_str[0] == 'd')
  {
    return(PM_PIN_PULLDOWN_100K);
  }
  if (pull_str[0] == 'f')
  {
    return(PM_PIN_UP_DOWN_FLOAT);
  }

  printf("Failed to parse pull: %c\r\n", pull_str[0]);
  reset_to_default_config();
}

char *seekUntil(char *cursor, char needle)
{
  while (*cursor != needle && *cursor != '\0')
  {
    cursor++;
  }
  return(cursor);
}

char *extractNextEntry(char **cursor)
{
  char *end = seekUntil(*cursor, ';');

  *end = '\0';
  char *res = *cursor;
  *cursor = end + 1;
  return(res);
}

u32 parseInt(const char *s)
{
  u32 n = 0;

  while ('0' <= *s && *s <= '9')
  {
    n = 10 * n + (*s++ - '0');
  }
  return(n);
}
