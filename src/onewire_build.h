#ifndef ONEWIRE_BUILD_H_
#define ONEWIRE_BUILD_H_

/*
 * 1-Wire is part of a firmware image only when that image's config string
 * has a W pin. Silicon Labs receives the value through silabs_config.h.
 * Telink and the host stub pass -DHAS_ONEWIRE.
 */
#ifdef HAL_SILABS
#include "silabs_config.h"
#endif

#ifndef HAS_ONEWIRE
#define HAS_ONEWIRE    0
#endif

#endif // ONEWIRE_BUILD_H_
