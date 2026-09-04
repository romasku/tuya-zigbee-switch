#include "hal/uart.h"
#include "stub/machine_io.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HAL_UART_STUB_RX_DEPTH 256
#define HAL_UART_STUB_TX_DEPTH 256

typedef struct
{
  uint8_t rx_buf[HAL_UART_STUB_RX_DEPTH];
  uint8_t tx_buf[HAL_UART_STUB_TX_DEPTH];
  uint16_t rx_head;
  uint16_t rx_tail;
  uint16_t tx_head;
  uint16_t tx_tail;
  uint8_t initialized;
} hal_uart_stub_t;

static hal_uart_stub_t g_hal_uart_stub;

static void ring_clear(hal_uart_stub_t *uart)
{
  uart->rx_head = 0;
  uart->rx_tail = 0;
  uart->tx_head = 0;
  uart->tx_tail = 0;
}

static uint16_t ring_used(const hal_uart_stub_t *uart, uint16_t head,
                          uint16_t tail)
{
  if (head >= tail)
  {
    return head - tail;
  }
  return (HAL_UART_STUB_RX_DEPTH - tail) + head;
}

static uint16_t ring_room(const hal_uart_stub_t *uart, uint16_t head,
                          uint16_t tail)
{
  return HAL_UART_STUB_RX_DEPTH - ring_used(uart, head, tail) - 1;
}

static void ring_push_byte(uint8_t *buf, uint16_t *head, uint16_t *tail,
                           uint16_t depth, uint8_t value)
{
  (void)tail;
  (void)depth;
  buf[*head] = value;
  (*head)++;
  if (*head >= depth)
  {
    *head = 0;
  }
}

static uint8_t ring_pop_byte(uint8_t *buf, uint16_t *head, uint16_t *tail,
                             uint16_t depth)
{
  uint8_t value = buf[*tail];
  (*tail)++;
  if (*tail >= depth)
  {
    *tail = 0;
  }
  return value;
}

void hal_uart_init(const hal_uart_config_t *cfg)
{
  memset(&g_hal_uart_stub, 0, sizeof(g_hal_uart_stub));
  g_hal_uart_stub.initialized = 1;

  if (cfg)
  {
    io_log("UART", "init baudrate=%lu data_bits=%u stop_bits=%u parity=%u",
           (unsigned long)cfg->baudrate, cfg->data_bits, cfg->stop_bits,
           cfg->parity);
  }
  else
  {
    io_log("UART", "init default config baudrate=115200");
  }
}

void hal_uart_deinit(void)
{
  memset(&g_hal_uart_stub, 0, sizeof(g_hal_uart_stub));
  io_log("UART", "deinit");
}

uint16_t hal_uart_rx_available(void)
{
  if (!g_hal_uart_stub.initialized)
  {
    return 0;
  }

  uint16_t used = 0;
  uint16_t head = g_hal_uart_stub.rx_head;
  uint16_t tail = g_hal_uart_stub.rx_tail;
  if (head >= tail)
  {
    used = head - tail;
  }
  else
  {
    used = (HAL_UART_STUB_RX_DEPTH - tail) + head;
  }
  return used;
}

void hal_uart_flush(void)
{
  ring_clear(&g_hal_uart_stub);
  io_log("UART", "flush");
}

hal_uart_status_t hal_uart_write(const uint8_t *data, uint16_t len,
                                 uint16_t *written)
{
  if (!data && len != 0)
  {
    return HAL_UART_ERR_INVALID_ARG;
  }
  if (!g_hal_uart_stub.initialized)
  {
    return HAL_UART_ERR_IO;
  }

  uint16_t actual = 0;
  for (uint16_t i = 0; i < len && actual < HAL_UART_STUB_TX_DEPTH; i++)
  {
    uint16_t tx_free = HAL_UART_STUB_TX_DEPTH -
                       (g_hal_uart_stub.tx_head >= g_hal_uart_stub.tx_tail
                            ? g_hal_uart_stub.tx_head - g_hal_uart_stub.tx_tail
                            : (HAL_UART_STUB_TX_DEPTH - g_hal_uart_stub.tx_tail) +
                                  g_hal_uart_stub.tx_head);
    if (tx_free == 0)
    {
      break;
    }

    g_hal_uart_stub.tx_buf[g_hal_uart_stub.tx_head] = data[i];
    g_hal_uart_stub.tx_head++;
    if (g_hal_uart_stub.tx_head >= HAL_UART_STUB_TX_DEPTH)
    {
      g_hal_uart_stub.tx_head = 0;
    }
    actual++;
  }

  if (written)
  {
    *written = actual;
  }

  io_log("UART", "write %u byte(s)", actual);
  return HAL_UART_OK;
}

hal_uart_status_t hal_uart_read(uint8_t *data, uint16_t len,
                                uint16_t *read_len)
{
  if (!data && len != 0)
  {
    return HAL_UART_ERR_INVALID_ARG;
  }
  if (!g_hal_uart_stub.initialized)
  {
    return HAL_UART_ERR_IO;
  }

  uint16_t actual = 0;
  while (actual < len && hal_uart_rx_available() > 0)
  {
    data[actual++] = ring_pop_byte(g_hal_uart_stub.rx_buf,
                                   &g_hal_uart_stub.rx_head,
                                   &g_hal_uart_stub.rx_tail,
                                   HAL_UART_STUB_RX_DEPTH);
  }

  if (read_len)
  {
    *read_len = actual;
  }

  if (actual > 0)
  {
    io_log("UART", "read %u byte(s)", actual);
  }

  return HAL_UART_OK;
}

hal_uart_status_t hal_uart_write_byte(uint8_t byte)
{
  uint16_t written = 0;
  return hal_uart_write(&byte, 1, &written);
}

hal_uart_status_t hal_uart_read_byte(uint8_t *byte)
{
  uint16_t read_len = 0;
  if (!byte)
  {
    return HAL_UART_ERR_INVALID_ARG;
  }
  return hal_uart_read(byte, 1, &read_len);
}

/* ---- test hooks -------------------------------------------------------- */
/* The Tuya secondary MCU bridge had no runtime coverage at all: everything was
 * asserted against the source text. These let a test drive both directions. */

void stub_uart_inject_rx(const uint8_t *data, uint16_t len)
{
  if (!data || !g_hal_uart_stub.initialized)
  {
    return;
  }
  for (uint16_t i = 0; i < len; i++)
  {
    if (ring_room(&g_hal_uart_stub, g_hal_uart_stub.rx_head,
                  g_hal_uart_stub.rx_tail) == 0)
    {
      break;
    }
    ring_push_byte(g_hal_uart_stub.rx_buf, &g_hal_uart_stub.rx_head,
                   &g_hal_uart_stub.rx_tail, HAL_UART_STUB_RX_DEPTH, data[i]);
  }
}

uint16_t stub_uart_take_tx(uint8_t *out, uint16_t max)
{
  uint16_t actual = 0;

  if (!out || !g_hal_uart_stub.initialized)
  {
    return 0;
  }
  while (actual < max &&
         ring_used(&g_hal_uart_stub, g_hal_uart_stub.tx_head,
                   g_hal_uart_stub.tx_tail) > 0)
  {
    out[actual++] = ring_pop_byte(g_hal_uart_stub.tx_buf,
                                  &g_hal_uart_stub.tx_head,
                                  &g_hal_uart_stub.tx_tail,
                                  HAL_UART_STUB_TX_DEPTH);
  }
  return actual;
}
