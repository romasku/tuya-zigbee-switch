#include "hal/uart.h"

void hal_uart_init(const hal_uart_config_t *cfg) {
    (void)cfg;
}

void hal_uart_deinit(void) {
}

uint16_t hal_uart_rx_dropped(void) {
    return 0;
}

uint16_t hal_uart_rx_available(void) {
    return 0;
}

void hal_uart_flush(void) {
}

/* No secondary-MCU UART on Silabs boards yet: say so instead of pretending
 * the bytes went out (the link counts failures and the TX queue backs off). */
hal_uart_status_t hal_uart_write(const uint8_t *data, uint16_t len,
                                 uint16_t *written) {
    if (written) {
        *written = 0;
    }
    (void)data;
    (void)len;
    return HAL_UART_ERR_IO;
}

hal_uart_status_t hal_uart_read(uint8_t *data, uint16_t len,
                                uint16_t *read_len) {
    if (read_len) {
        *read_len = 0;
    }
    (void)data;
    (void)len;
    return HAL_UART_OK;
}

hal_uart_status_t hal_uart_write_byte(uint8_t byte) {
    (void)byte;
    return HAL_UART_ERR_IO;
}

hal_uart_status_t hal_uart_read_byte(uint8_t *byte) {
    if (byte) {
        *byte = 0;
    }
    return HAL_UART_OK;
}
