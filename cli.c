#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cli.h"

struct char_list {
	char c;
	struct list_head list;
};

static void cli_init_node(struct cli_tree *node)
{
	node->c = 0;
	node->func  = NULL;
	node->ptr   = NULL;
	node->help  = NULL;
	node->child = NULL;
	INIT_LIST_HEAD(&node->list);
}

struct cli_tree* cli_tree_create()
{

	struct cli_tree *node = malloc(sizeof(*node));
	if (node == NULL)
		return NULL;
	cli_init_node(node);
	return node;
}

void cli_tree_destroy(struct cli_tree *root)
{
	struct cli_tree *node, *tmp;
	list_for_each_entry_safe(node, tmp, &root->list, list) {
		if (node->child)
			cli_tree_destroy(node->child);
		list_del(&node->list);
		free(node);
	}
	free(root);
}

int cli_add_cmd(struct cli_tree *root, char *cmd, int (*func)(void*, char*), void* ptr, char *help)
{
	assert(root != NULL);
	assert(cmd[0] != '\0');
	struct cli_tree *node;

	if (cmd[1] == '\0') {
		/* As there is only one letter left, we know we need to allocate
		 * the command now */

		list_for_each_entry(node, &root->list, list) {
			if (node->c == cmd[0]) {
				/* Replace an old command with the same string */
				node->func = func;
				node->ptr  = ptr;
				node->help = help;
				return 0;
			}
		}

		/* The new command has a unique string */
		node = cli_tree_create();
		if (node == NULL)
			return 1;

		node->func = func;
		node->ptr  = ptr;
		node->help = help;
		node->c = cmd[0];
		list_add_tail(&node->list, &root->list);
		return 0;

	} else {
		/* As there is more than one letter left, we just allocate tree nodes and continue parsing */

		list_for_each_entry(node, &root->list, list) {
			if (node->c == cmd[0]) {
				/* This means a node was found with the current letter in the new command,
				 * allocate a new entry in the linked list and continue with the next letter. */
				if (node->child == NULL)
					node->child = cli_tree_create();
				if (node->child == NULL)
					return 1;
				return cli_add_cmd(node->child, cmd + 1, func, ptr, help);
			}
		}

		/* No node with the current letter in the new command was found, this means
		 * we have to start allocating new nodes. This also means the above loop will never trigger
		 * from now on, but we don't care about that. */
		node = cli_tree_create();
		if (node == NULL)
			return 1;

		node->c = cmd[0];
		node->child = cli_tree_create();
		if (node->child == NULL)
			return 1;

		list_add_tail(&node->list, &root->list);
		cli_add_cmd(node->child, cmd + 1, func, ptr, help);
		return 0;
	}
}

static struct cli_tree* cli_find_node(struct cli_tree *root, char *cmd)
{
	assert(root != NULL);
	struct cli_tree *node;
	if (cmd[0] != '\0') {
		list_for_each_entry(node, &root->list, list) {
			if (node->c == cmd[0]) {
				if (cmd[1] == '\0')
					return node;
				else if (node->child)
					return cli_find_node(node->child, cmd + 1);
				else
					return NULL;
			}
		}
	}
	return NULL;
}

/* cli_rm_cmd removes a command from the command tree. It does not, however,
 * remove any tree nodes: this is a feature to keep the number of malloc()/free()s
 * to a sane level, as the cli trees for logged in users are constantly in a
 * state of flux. */
int cli_rm_cmd(struct cli_tree *root, char *string)
{
	struct cli_tree *node = cli_find_node(root, string);
	if (node == NULL)
		return 1;
	node->func = NULL;
	node->ptr  = NULL;
	node->help = NULL;
	return 0;
}

int cli_run_cmd(struct cli_tree *root, char *string)
{
	int ret;
	unsigned int i;
	unsigned int len = strlen(string);
	for (i = 0; i < len && string[i] != ' '; i++);

	char *cmd = alloca(i + 1);
	memcpy(cmd, string, i);
	cmd[i] = '\0';

	char *param = NULL;
	if (i < len - 1) {
		param = alloca(len - i + 1);
		memcpy(param, string + i + 1, len - i);
		param[len - i] = '\0';
	}

	struct cli_tree *node = cli_find_node(root, cmd);
	if (node && node->func)
		ret = node->func(node->ptr, param);
	else
		ret = -1;

	return ret;
}

static char* char_list_to_string(struct char_list *cl)
{
	unsigned int len = 0;
	struct list_head *lh;
	list_for_each(lh, &cl->list)
		len++;

	char *string = malloc(len + 1);
	if (string == NULL)
		return NULL;

	len = 0;
	struct char_list *node;
	list_for_each_entry(node, &cl->list, list)
		string[len++] = node->c;
	string[len] = '\0';

	return string;
}

static void __cli_print_help(FILE *f, struct char_list *cl_root, struct cli_tree *root)
{
	struct cli_tree *node;
	list_for_each_entry(node, &root->list, list) {
		struct char_list cl;
		cl.c = node->c;
		list_add_tail(&cl.list, &cl_root->list);

		if (node->func) {
			char *cmd = char_list_to_string(cl_root);
			if (node->help)
				fprintf(f, "%-10s %s\n", cmd, node->help);
			else
				fprintf(f, "%-10s No help text available\n", cmd);
			free(cmd);
		}

		if (node->child)
			__cli_print_help(f, cl_root, node->child);

		list_del(&cl.list);
	}
}

void cli_print_help(FILE *f, struct cli_tree *root)
{
	struct cli_tree *node;
	struct char_list cl_root;
	INIT_LIST_HEAD(&cl_root.list);
	__cli_print_help(f, &cl_root, root);
}
