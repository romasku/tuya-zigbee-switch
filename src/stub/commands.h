#ifndef COMMANDS_H
#define COMMANDS_H

#include "simple_repl.h"
#include "app.h"

/* expose table for REPL */
const SimpleReplCommand *commands_table(void);
size_t commands_count(void);

/* optional: a help printer that uses app_print_help() */
void commands_print_help(void);

#endif
