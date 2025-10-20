#ifndef APP_MACHINE_IO_H
#define APP_MACHINE_IO_H
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

extern bool g_machine_mode;

static inline void io_log(const char *prefix, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "[%s] ", prefix);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

static inline void io_res_ok(const char *fmt, ...) {
  if (!g_machine_mode)
    return;
  fputs("RES OK", stdout);
  if (fmt && *fmt) {
    fputc(' ', stdout);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
  }
  fputc('\n', stdout);
  fflush(stdout);
}

static inline void io_res_err(const char *fmt, ...) {
  if (!g_machine_mode)
    return;
  fputs("RES ERR ", stdout);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  fputc('\n', stdout);
  fflush(stdout);
}

static inline void io_evt(const char *fmt, ...) {
  if (!g_machine_mode)
    return;
  fputs("EVT ", stdout);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  fputc('\n', stdout);
  fflush(stdout);
}
#endif
