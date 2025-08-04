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
  nv_flashWriteNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG, sizeof(config.data), (u8 *)config.data);
}

void device_config_remove_from_nv()
{
  nv_flashSingleItemRemove(NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG, sizeof(config.data));
}

void device_config_read_from_nv()
{
  nv_sts_t st = nv_flashReadNew(1, NV_MODULE_ZCL, NV_ITEM_ZCL_DEVICE_CONFIG, sizeof(config.data), (u8 *)config.data);

  if (st != NV_SUCC)
  {
    memcpy(config.data, default_config_data, sizeof(default_config_data));
  }
  config.size = strlen(config.data);
  printf("Loaded config %s\r\n", config.data);
}
