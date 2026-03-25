#include "hal/zigbee.h"
#include "hal/timer.h"
#include "device_config/config_parser.h"
#include "hal/printf_selector.h"


struct {
    uint16_t short_poll_rate_ms;
    uint16_t long_poll_rate_ms;
    uint16_t go_to_long_poll_timeout_ms;
    uint32_t last_activity_ms;
    bool     in_long_poll;
} poll_rate_controller = {
    .short_poll_rate_ms         =   250,
    .long_poll_rate_ms          =  5000,
    .go_to_long_poll_timeout_ms = 20000,
    .in_long_poll               = false,
};

void poll_rate_controller_update();

void on_zcl_activity(void) {
    printf("ZCL activity detected, resetting poll rate to short poll\r\n");
    poll_rate_controller.last_activity_ms = hal_millis();
    poll_rate_controller.in_long_poll     = false;
    poll_rate_controller_update();
}

void poll_rate_controller_init() {
    hal_zigbee_register_on_zcl_activity_callback(on_zcl_activity);
    poll_rate_controller.last_activity_ms = hal_millis();
    poll_rate_controller.in_long_poll     = false;
}

void poll_rate_controller_update() {
    if (hal_millis() - poll_rate_controller.last_activity_ms >=
        poll_rate_controller.go_to_long_poll_timeout_ms) {
        poll_rate_controller.in_long_poll = true;
    }
    uint32_t desired_poll_rate_ms = poll_rate_controller.in_long_poll
                                      ? poll_rate_controller.long_poll_rate_ms
                                      : poll_rate_controller.short_poll_rate_ms;
    if (hal_zigbee_get_poll_rate_ms() != desired_poll_rate_ms) {
        hal_zigbee_set_poll_rate_ms(desired_poll_rate_ms);
    }
}
