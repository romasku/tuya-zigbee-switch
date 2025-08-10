#include "zcl_include.h"
#include "configs/nv_slots_cfg.h"


#ifndef STRINGIFY
#define _STRINGIFY(x)    #x
#define STRINGIFY(x)     _STRINGIFY(x)
#endif

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG    UNKNOWN; UNKNOWN;
#endif

const char default_config_data[] = STRINGIFY(DEFAULT_CONFIG);


struct
{
  u16  size;
  char data[256];
} config;


void device_config_write_to_nv()
{
  nv_sts_t st = 0;
  st = nv_flashWriteNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG_SIZE, sizeof(config.size), (u8 *)&config.size);

  if(st != NV_SUCC)
  {
    printf("Failed to write DEVICE_CONFIG_SIZE to NV, st: %d. (bytes: %d, value: %d)\r\n", st, sizeof(config.size), config.size);
    return;
  }

  st = nv_flashWriteNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG_DATA, config.size, (u8 *)config.data);

  if(st != NV_SUCC)
  {
    printf("Failed to write DEVICE_CONFIG_DATA to NV, st: %d. (bytes: %d)\r\n", st, config.size);
  }
}

void device_config_remove_from_nv()
{
  nv_flashSingleItemRemove(NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG_DATA, config.size);
  nv_flashSingleItemRemove(NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG_SIZE, sizeof(config.size));
}

void device_config_read_from_nv()
{
  nv_sts_t st = 0;
  st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG_SIZE, sizeof(config.size), (u8 *)&config.size);

  if (st != NV_SUCC)
  {
    printf("Failed to read DEVICE_CONFIG_SIZE from NV, using default config instead, st: %d. (bytes: %d)\r\n", st, sizeof(config.size));
    memcpy(config.data, default_config_data, sizeof(default_config_data));
    config.size = strlen(config.data);
    return;
  }

  st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG_DATA, config.size, (u8 *)config.data);

  if (st != NV_SUCC)
  {
    printf("Failed to read DEVICE_CONFIG_DATA from NV, using default config instead, st: %d. (bytes: %d)\r\n", st, config.size);
    memcpy(config.data, default_config_data, sizeof(default_config_data));
    config.size = strlen(config.data);
    return;
  }

  printf("Read config: %d chars from\r\n%s\r\n", config.size, config.data);
}