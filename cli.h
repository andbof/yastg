#ifndef _HAS_CLI_H
#define _HAS_CLI_H

#include <stdio.h>
#include "stringtrie.h"

void cli_tree_destroy(struct st_root *root);

int cli_add_cmd(struct st_root *root, char *cmd, int (*func)(void*, char*), void *ptr, char *help);
int cli_rm_cmd(struct st_root *root, char *cmd);
int cli_run_cmd(struct st_root * const root, const char * const string);

void cli_print_help(struct st_root *root, void (*print)(void*, const char*, ...),
		void *hints);

#endif
