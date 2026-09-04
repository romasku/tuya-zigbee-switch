#include "time_cluster.h"
#include "cluster_common.h"
#include "consts.h"
#include "hal/printf_selector.h"
#include "hal/timer.h"
#include "hal/zigbee.h"
#include <stdint.h>

/* ZCL counts from 2000-01-01, Unix from 1970-01-01. */
#define ZIGBEE_EPOCH_OFFSET    946684800u

/* Written by the coordinator. */
uint32_t zcl_time       = 0;
uint32_t zcl_local_time = 0;
/* Bit 0 Master, bit 1 Synchronized. We are neither a master clock nor
   authoritative, so only Synchronized is raised, and only once written. */
uint8_t  zcl_time_status = 0;

static uint32_t base_utc    = 0; /* value at the moment it was written */
static uint32_t base_millis = 0;

static const uint16_t cluster_revision_time = 0x0001;

void time_cluster_on_write_attr(uint16_t attribute_id)
{
    if (attribute_id != ZCL_ATTR_TIME_TIME &&
        attribute_id != ZCL_ATTR_TIME_LOCAL_TIME)
    {
        return;
    }
    if (zcl_time == 0)
    {
        return; /* a write of zero carries no information */
    }
    base_utc        = zcl_time;
    base_millis     = hal_millis();
    zcl_time_status = 0x02; /* Synchronized */
    printf("Clock set from coordinator: %u\r\n", (unsigned)zcl_time);
}

uint32_t zigbee_time_unix(void)
{
    if (base_utc == 0)
    {
        return 0;
    }
    /* Unsigned arithmetic, so this stays correct across a millis wrap. */
    uint32_t elapsed_s = (hal_millis() - base_millis) / 1000u;
    return base_utc + ZIGBEE_EPOCH_OFFSET + elapsed_s;
}

void time_cluster_add_to_endpoint(zigbee_time_cluster *cluster,
                                  hal_zigbee_endpoint *endpoint)
{
    SETUP_ATTR(0, ZCL_ATTR_TIME_TIME, ZCL_DATA_TYPE_UTC, ATTR_WRITABLE,
               zcl_time);
    SETUP_ATTR(1, ZCL_ATTR_TIME_STATUS, ZCL_DATA_TYPE_BITMAP8, ATTR_READONLY,
               zcl_time_status);
    SETUP_ATTR(2, ZCL_ATTR_TIME_LOCAL_TIME, ZCL_DATA_TYPE_UINT32,
               ATTR_WRITABLE, zcl_local_time);
    SETUP_ATTR(3, ZCL_ATTR_GLOBAL_CLUSTER_REVISION, ZCL_DATA_TYPE_UINT16,
               ATTR_READONLY, cluster_revision_time);

    endpoint->clusters[endpoint->cluster_count].cluster_id      = ZCL_CLUSTER_TIME;
    endpoint->clusters[endpoint->cluster_count].attribute_count = 4;
    endpoint->clusters[endpoint->cluster_count].attributes      = cluster->attr_infos;
    endpoint->clusters[endpoint->cluster_count].is_server       = 1;
    endpoint->cluster_count++;
}
