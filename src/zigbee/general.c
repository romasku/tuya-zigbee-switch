#include "tl_common.h"
#include "zb_common.h"
#include "zcl_include.h"



void factoryReset() {
    zb_factoryReset();
	tl_bdbReset2FN();
	zb_resetDevice();
}