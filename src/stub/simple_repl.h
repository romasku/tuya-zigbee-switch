#ifndef SIMPLE_REPL_H
#define SIMPLE_REPL_H

#include <stddef.h>
#include <signal.h>

typedef int (*repl_cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    repl_cmd_fn fn;
} SimpleReplCommand;

typedef struct {
    const SimpleReplCommand *commands;
    size_t                   command_count;

    void (*poll_cb)(void *poll_user);
    void *                   poll_user;

    volatile sig_atomic_t *  should_exit;
} SimpleReplConfig;

int simple_repl_run(const SimpleReplConfig *cfg);

int simple_repl_dispatch_line(const SimpleReplConfig *cfg, char *line);

#endif /* SIMPLE_REPL_H */
