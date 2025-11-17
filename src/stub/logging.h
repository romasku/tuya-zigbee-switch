#ifndef STUB_LOGGING_H
#define STUB_LOGGING_H

#include <stdarg.h>
#include <stdio.h>

static inline void io_log(const char *prefix, const char *format, ...) {
  va_list args;
  va_start(args, format);
  printf("[%s] ", prefix);
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

#endif