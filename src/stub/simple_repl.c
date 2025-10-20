#include "simple_repl.h"

#include "machine_io.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#ifndef SIMPLE_REPL_MAX_LINE
#define SIMPLE_REPL_MAX_LINE 256
#endif

static const char kPrompt[] = "> ";
static const unsigned kPollMs = 1; /* 1 ms polling */

static char *ltrim_(char *s) {
  while (*s && isspace((unsigned char)*s))
    s++;
  return s;
}
static void rtrim_(char *s) {
  char *e = s + strlen(s);
  while (e > s && isspace((unsigned char)e[-1]))
    --e;
  *e = '\0';
}
static int split_tokens_(char *line, char **argv, int max_tokens) {
  int n = 0;
  char *p = ltrim_((line));
  while (*p && n < max_tokens) {
    argv[n++] = p;
    while (*p && !isspace((unsigned char)*p))
      p++;
    if (*p) {
      *p++ = '\0';
      p = ltrim_(p);
    }
  }
  return n;
}

static int dispatch_tokens_(const SimpleReplConfig *cfg, int argc,
                            char **argv) {
  if (!cfg || !cfg->commands || cfg->command_count == 0 || argc == 0)
    return 0;
  for (size_t i = 0; i < cfg->command_count; ++i) {
    if (strcmp(argv[0], cfg->commands[i].name) == 0) {
      return cfg->commands[i].fn(argc, argv);
    }
  }
  fprintf(stdout, "Unknown command '%s'.\n", argv[0]);
  io_res_err("unknown_cmd=%s", argv[0]);
  fflush(stdout);
  return -1;
}

/* ======== public API ======== */

int simple_repl_dispatch_line(const SimpleReplConfig *cfg, char *line) {
  if (!cfg || !line)
    return -1;
  rtrim_(line);
  char *argv[16];
  int argc = split_tokens_(line, argv, (int)(sizeof(argv) / sizeof(argv[0])));
  if (argc == 0)
    return 0;
  return dispatch_tokens_(cfg, argc, argv);
}

int simple_repl_run(const SimpleReplConfig *cfg) {
  if (!cfg)
    return -1;

  /* initial prompt if interactive */
  if (isatty(STDIN_FILENO)) {
    fputs(kPrompt, stdout);
    fflush(stdout);
  }

  char line[SIMPLE_REPL_MAX_LINE];

  const int tty = isatty(STDIN_FILENO);

  for (;;) {
    if (cfg->should_exit && *cfg->should_exit)
      break;

    if (cfg->poll_cb)
      cfg->poll_cb(cfg->poll_user);

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    struct timeval tv = {.tv_sec = 0, .tv_usec = (suseconds_t)(kPollMs * 1000)};

    int sel = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
    if (sel <= 0) { /* timeout or error */
      if (sel < 0 && errno != EINTR) {
        perror("select");
        return 1;
      }
      continue; /* on timeout or EINTR */
    }

    if (!fgets(line, sizeof line, stdin)) {
      if (feof(stdin)) {
        if (tty)
          fputc('\n', stdout);
        break;
      }
      clearerr(stdin);
      continue;
    }

    simple_repl_dispatch_line(cfg, line);

    if (tty) {
      fputs(kPrompt, stdout);
      fflush(stdout);
    }
  }

  return 0;
}