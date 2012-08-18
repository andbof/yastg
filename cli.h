#ifndef _HAS_CLI_H
#define _HAS_CLI_H

#include <stdio.h>
#include "list.h"

/* The cli_tree struct will look somewhat like this for the "echo" command,
 * all boxes represent one cli_tree struct. If a parameter is not listed, assume it is 0 or NULL.
 * The "->" arrows represents a linked list relationship.
 * 
 * +------+  +----------------------------+  +----------------------------+
 * | root |->| list, c = 'e', func = NULL |->| list for another letter... |
 * +------+  +----------------------------+  +----------------------------+
 *  |child       |
 *   =NULL   +-------+  +----------------------------+
 *           | child |->| list, c = 'c', func = NULL |->...
 *           +-------+  +----------------------------+
 *            |child                |
 *             =NULL            +-------+  +----------------------------+
 *                              | child |->| list, c = 'h', func = NULL |
 *                              +-------+  +----------------------------+
 *                               |child                |
 *                                =NULL            +-------+  +-----------------------------+
 *                                                 | child |->| list, c = 'o', func = &echo |
 *                                                 +-------+  +-----------------------------+
 *                                                  |child = NULL        |child = NULL
 */
struct cli_tree {
	char c;
	int (*func)(void*, char*);
	void *ptr;
	char *help;
	struct cli_tree *child;
	struct list_head list;
};

struct cli_tree* cli_tree_create();
void cli_tree_destroy(struct cli_tree *root);

int cli_add_cmd(struct cli_tree *root, char *cmd, int (*func)(void*, char*), void *ptr, char *help);
int cli_rm_cmd(struct cli_tree *root, char *string);
int cli_run_cmd(struct cli_tree *root, char *string);

void cli_print_help(FILE *f, struct cli_tree *root);

#endif
