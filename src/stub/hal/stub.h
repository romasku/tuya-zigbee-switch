#ifndef _HAL_STUB_H_
#define _HAL_STUB_H_

#include "hal/gpio.h"
#include "hal/zigbee.h"
#include <stdint.h>

// GPIO stub functions
void stub_gpio_enable_debug(int enable);
void stub_gpio_simulate_input(hal_gpio_pin_t gpio_pin, uint8_t value);
uint8_t stub_gpio_get_output(hal_gpio_pin_t gpio_pin);

// GPIO counter stub functions
void stub_set_pulse_counter(hal_gpio_pin_t gpio_pin, uint32_t value);

// Tasks stub functions
void stub_tasks_poll(void);

// NVM stub functions
void stub_nvm_enable_debug(int enable);
void stub_nvm_set_data_dir(const char *dir);

// Zigbee stub functions
void stub_zigbee_enable_debug(int enable);
void stub_zigbee_set_network_status(hal_zigbee_network_status_t status);
void stub_zigbee_add_binding(uint16_t short_addr, uint8_t endpoint,
                             uint16_t cluster_id);
void stub_zigbee_clear_bindings(void);
hal_zigbee_endpoint *stub_zigbee_get_endpoints(uint8_t *count);
hal_zigbee_cmd_result_t stub_zigbee_simulate_command(uint8_t endpoint,
                                                     uint16_t cluster_id,
                                                     uint8_t command_id,
                                                     void *payload);
void stub_simulate_zigbee_attribute_write(uint8_t endpoint, uint16_t cluster_id,
                                          uint16_t attribute_id);

// Millis stub functions
void stub_millis_init();
void stub_millis_freeze();
void stub_millis_unfreeze();
void stub_millis_step(uint64_t step);

#endif // _HAL_STUB_H_
