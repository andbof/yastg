#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include "stringtree.h"

struct cli_data {
	int (*func)(void*, char*);
	char *help;
	void *data;
};

void cli_tree_destroy(struct list_head *root)
{
	st_destroy(root, ST_DO_FREE_DATA);
}

static char* trim_and_validate(char *string)
{
	size_t last;

	if (!string || string[0] == '\0')
		return NULL;

	while (isspace(*string))
		string++;

	last = strlen(string);
	if (!last)
		return NULL;

	last--;
	while (isspace(string[last])) {
		string[last] = '\0';
		if (last)
			last--;
		else
			break;
	}

	if (!*string)
		return NULL;

	return string;
}

int cli_add_cmd(struct list_head *root, char *cmd, int (*func)(void*, char*), void *ptr, char *help)
{
	struct cli_data *node;

	cmd = trim_and_validate(cmd);
	if (!cmd)
		return -1;

	node = malloc(sizeof(*node));
	if (!node)
		return -1;

	node->data = ptr;
	node->help = help;
	node->func = func;

	if (st_add_string(root, cmd, node)) {
		free(node);
		return -1;
	}

	return 0;
}

/*
 * cli_rm_cmd removes a command from the command tree. It does not, however,
 * remove any tree nodes: this is a feature to keep the number of malloc()/free()s
 * to a sane level, as the cli trees for logged in users are constantly in a
 * state of flux and the nodes will probably be reused very soon.
 */
int cli_rm_cmd(struct list_head *root, char *cmd)
{
	struct st_node *node;

	node = st_rm_string(root, cmd);
	if (!node)
		return -1;

	free(node);
	return 0;
}

int cli_run_cmd(struct list_head *root, char *string)
{
	int ret;
	unsigned int i, len;
	char *cmd;
	char *param;
	struct cli_data *node;

	string = trim_and_validate(string);
	if (!string)
		return -1;

	len = strlen(string);

	for (i = 0; i < len && string[i] != ' '; i++);

	cmd = alloca(i + 1);
	memcpy(cmd, string, i);
	cmd[i] = '\0';

	param = NULL;
	if (i < len - 1) {
		param = alloca(len - i + 1);
		memcpy(param, string + i + 1, len - i);
		param[len - i] = '\0';
	}

	node = st_lookup_string(root, cmd);
	if (node && node->func)
		ret = node->func(node->data, param);
	else
		ret = -1;

	return ret;
}

static void __cli_print_help(FILE *f, struct list_head *root, char *buf, size_t idx, const size_t len)
{
	struct st_node *st;
	struct cli_data *cli;

	if (len - idx < 2)
		return;

	buf[idx + 1] = '\0';

	list_for_each_entry(st, root, list) {
		buf[idx] = st->c;
		if (st->data) {
			cli = st->data;
			if (cli->func) {
				if (cli->help)
					fprintf(f, "%-10s %s\n", buf, cli->help);
				else
					fprintf(f, "%-10s No help text available\n", buf);
			}
		}
	}

	list_for_each_entry(st, root, list) {
		buf[idx] = st->c;
		__cli_print_help(f, &st->children, buf, idx + 1, len);
	}
}

#define MAX_CMD_LEN 64
void cli_print_help(FILE *f, struct list_head *root)
{
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof(buf));

	__cli_print_help(f, root, buf, 0, MAX_CMD_LEN);
}
