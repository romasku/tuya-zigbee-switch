#include "tl_common.h"
#include "configs/nv_slots_cfg.h"
#include "common/types.h"
#include "device_config.h"


#ifndef STRINGIFY
#define _STRINGIFY(x)    #x
#define STRINGIFY(x)     _STRINGIFY(x)
#endif

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG    UNKNOWN; UNKNOWN;
#endif

const char default_config_data[] = STRINGIFY(DEFAULT_CONFIG);


device_config_str_t device_config_str;


void device_config_write_to_nv()
{
  printf("Writing config to nv: %s\r\n", device_config_str.data);
  nv_sts_t st = 0;
 

  printf("Size: %d\r\n", sizeof(device_config_str));
  st = nv_flashWriteNew(1, NV_MODULE_APP, NV_ITEM_DEVICE_CONFIG, sizeof(device_config_str), (u8 *)&device_config_str);

  if(st != NV_SUCC)
  {
    printf("Failed to write DEVICE_CONFIG_DATA to NV, st: %d. (bytes: %d)\r\n", st, device_config_str.size);
  } else {
    printf("success!\r\n");
  }
}

void device_config_remove_from_nv()
{
  nv_flashSingleItemRemove(NV_MODULE_APP, NV_ITEM_DEVICE_CONFIG, sizeof(device_config_str));
}

void device_config_read_from_nv()
{
  nv_sts_t st = 0;

  st = nv_flashReadNew(1, NV_MODULE_APP, NV_ITEM_DEVICE_CONFIG, sizeof(device_config_str), (u8 *)&device_config_str);

  if (st != NV_SUCC)
  {
    printf("Failed to read DEVICE_CONFIG_DATA from NV, using default config instead, st: %d. (bytes: %d)\r\n", st, device_config_str.size);
    memcpy(device_config_str.data, default_config_data, sizeof(default_config_data));
    device_config_str.size = strlen(device_config_str.data);
    return;
  }

  printf("Read config: %d chars from\r\n%s\r\n", device_config_str.size, device_config_str.data);
}