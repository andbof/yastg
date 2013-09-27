#ifndef _HAS_CLI_H
#define _HAS_CLI_H

#include <stdio.h>
#include "list.h"

void cli_tree_destroy(struct list_head *root);

int cli_add_cmd(struct list_head *root, char *cmd, int (*func)(void*, char*), void *ptr, char *help);
int cli_rm_cmd(struct list_head *root, char *cmd);
int cli_run_cmd(struct list_head * const root, const char * const string);

void cli_print_help(struct list_head *root, void (*print)(void*, const char*, ...),
		void *hints);

#endif
