#ifndef _HAL_SYSTEM_H_
#define _HAL_SYSTEM_H_

/**
 * Reset the system/microcontroller
 */
void __attribute__((noreturn)) hal_system_reset(void);

void hal_factory_reset(void);

#endif /* _HAL_SYSTEM_H_ */