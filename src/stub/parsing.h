#ifndef PARSING_H
#define PARSING_H

#include <stdint.h>
#include <stdlib.h>

static int parse_u8_dec(const char *s, uint8_t *out) {
  char *e = NULL;
  long v = strtol(s, &e, 10);
  if (*s == '\0' || *e || v < 0 || v > 255)
    return -1;
  *out = (uint8_t)v;
  return 0;
}
static int parse_u16_hex(const char *s, uint16_t *out) {
  char *e = NULL;
  long v = strtol(s, &e, 16);
  if (*s == '\0' || *e || v < 0 || v > 0xFFFF)
    return -1;
  *out = (uint16_t)v;
  return 0;
}
static int parse_u32_dec(const char *s, uint32_t *out) {
  char *e = NULL;
  unsigned long v = strtoul(s, &e, 10);
  if (*s == '\0' || *e)
    return -1;
  *out = (uint32_t)v;
  return 0;
}
static void bytes_to_hexstr(const uint8_t *bytes, size_t len, char *out) {
  static const char HEX_DIGITS[] = "0123456789ABCDEF";

  for (size_t i = 0; i < len; ++i) {
    uint8_t b = bytes[i];
    out[2 * i] = HEX_DIGITS[b >> 4];
    out[2 * i + 1] = HEX_DIGITS[b & 0x0F];
  }
  out[2 * len] = '\0';
}

static int hexchar_to_val(char c, uint8_t *out) {
  if ('0' <= c && c <= '9')
    *out = c - '0';
  else if ('A' <= c && c <= 'F')
    *out = c - 'A' + 10;
  else if ('a' <= c && c <= 'f')
    *out = c - 'a' + 10;
  else
    return -1;
  return 0;
}

// Converts hex string to bytes. Returns 0 on success, -1 on error.
static int hexstr_to_bytes(const char *hexstr, uint8_t *bytes, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    uint8_t hi, lo;
    if (hexchar_to_val(hexstr[2 * i], &hi) < 0 ||
        hexchar_to_val(hexstr[2 * i + 1], &lo) < 0)
      return -1;

    bytes[i] = (hi << 4) | lo;
  }
  return 0;
}

#endif // PARSING_H