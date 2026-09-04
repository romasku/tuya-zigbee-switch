#ifndef _HAL_UART_H_
#define _HAL_UART_H_

#include <stdint.h>
#include <stddef.h>

/** UART transport status codes */
typedef enum
{
  HAL_UART_OK = 0,
  HAL_UART_ERR_INVALID_ARG = -1,
  HAL_UART_ERR_BUSY = -2,
  HAL_UART_ERR_IO = -3,
} hal_uart_status_t;

/** Common UART capabilities that the application can use for a secondary MCU. */
typedef struct
{
  uint32_t baudrate;
  uint8_t data_bits;
  uint8_t stop_bits;
  uint8_t parity;
  /* Pin ids for the UART towards the secondary MCU. 0 = use platform default.
     On Telink these must be one of the UART pinmux options:
       TX: PA2 PB1 PC2 PD0 PD3 PD7   RX: PA0 PB0 PB7 PC3 PC5 PD6            */
  uint32_t tx_pin;
  uint32_t rx_pin;
} hal_uart_config_t;

/**
 * Initialize a UART instance touching the secondary MCU.
 *
 * The application can leave the structure uninitialized for a fixed default
 * configuration if the platform defines one.
 */
void hal_uart_init(const hal_uart_config_t *cfg);

/** Release any UART-specific resources. */
void hal_uart_deinit(void);

/** Return the number of bytes that can be drained from the UART RX queue. */
uint16_t hal_uart_rx_available(void);

/** Empty the RX and TX queues for a UART instance. */
void hal_uart_flush(void);

/** Transmit a raw UART payload. */
hal_uart_status_t hal_uart_write(const uint8_t *data, uint16_t len,
                                 uint16_t *written);

/** Receive a raw UART payload. */
hal_uart_status_t hal_uart_read(uint8_t *data, uint16_t len,
                                uint16_t *read_len);

/** Convenience wrappers for single-byte traffic. */
hal_uart_status_t hal_uart_write_byte(uint8_t byte);
hal_uart_status_t hal_uart_read_byte(uint8_t *byte);

#endif /* _HAL_UART_H_ */
