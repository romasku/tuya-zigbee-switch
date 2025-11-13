
#include "reset.h"
#include "hal/nvm.h"
#include "hal/printf_selector.h"
#include "hal/system.h"

__attribute__((noreturn)) void reset_all() {
  printf("RESET ALL!\r\n");
  hal_nvm_clear_all();
  hal_factory_reset();
  hal_system_reset();
}