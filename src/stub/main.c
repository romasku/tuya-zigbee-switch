#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#ifndef HAL_STUB
#define HAL_STUB
#endif

#include "commands.h"
#include "simple_repl.h"
#include "stub/hal/stub.h"
#include "stub_app.h"

volatile sig_atomic_t g_should_exit = 0;
static void on_sigint(int sig) {
    (void)sig;
    g_should_exit = 1;
}

static void poll_wrapper(void *u) {
    (void)u;
    stub_app_poll();
}

static void print_usage(const char *prog) {
    printf("Usage: %s [--device-config <string>] [--help]\n", prog);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGINT, on_sigint);

    const struct option long_opts[] = {
        { "device-config", required_argument, 0, 'd' },
        { "not-joined",    no_argument,       0, 'j' },
        { "freeze-time",   no_argument,       0, 'f' },
        { "help",          no_argument,       0, 'h' },
        {               0,                 0, 0,   0 }
    };

    char device_conf_buf[APP_DEVICE_CONF_MAX];
    device_conf_buf[0] = '\0';
    bool joined = true;
    for (;;) {
        int opt = getopt_long(argc, argv, "d:j:f:h", long_opts, NULL);
        if (opt == -1)
            break;
        switch (opt) {
        case 'd':
            snprintf(device_conf_buf, sizeof(device_conf_buf), "%s", optarg);
            break;
        case 'j':
            joined = false;
            break;
        case 'f':
            stub_millis_freeze();
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            return 0;
        }
    }

    stub_app_init(device_conf_buf[0] ? device_conf_buf : NULL, joined);

    puts("[STUB] Entering interactive mode. Type 'h' for help.");
    commands_print_help(); // auto-generated from table

    SimpleReplConfig cfg = {
        .commands      = commands_table(),
        .command_count = commands_count(),
        .poll_cb       = poll_wrapper,
        .poll_user     = NULL,
        .should_exit   = &g_should_exit,
    };
    (void)simple_repl_run(&cfg);

    stub_app_shutdown();
    return 0;
}
