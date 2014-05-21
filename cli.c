#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include "common.h"
#include "stringtrie.h"

struct cli_data {
	int (*func)(void*, char*);
	char *help;
	void *data;
};

void cli_tree_destroy(struct st_root *root)
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

int cli_add_cmd(struct st_root *root, char *cmd, int (*func)(void*, char*), void *ptr, char *help)
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

int cli_rm_cmd(struct st_root *root, char *cmd)
{
	struct st_node *node;

	node = st_rm_string(root, cmd);
	if (!node)
		return -1;

	free(node);
	return 0;
}

int cli_run_cmd(struct st_root * const root, const char * const string)
{
	int r;
	unsigned int i, len;
	char *cmd, *param;
	char *line = NULL;
	struct cli_data *node;

	if (!string)
		return -1;

	line = strdup(string);
	if (!line)
		return -1;

	cmd = trim_and_validate(line);
	if (!cmd) {
		free(line);
		return -1;
	}

	len = strlen(cmd);

	for (i = 0; i < len && !isspace(line[i]); i++);
	line[i] = '\0';

	if (i < len - 1)
		param = trim_and_validate(cmd + i + 1);
	else
		param = NULL;

	node = st_lookup_string(root, cmd);
	if (node && node->func)
		r = node->func(node->data, param);
	else
		r = -1;

	free(line);
	return r;
}

struct cli_print {
	void (*print)(void*, const char*, ...);
	void *hints;
};

void __cli_print_help(void *st_data, const char *cmd_name, void *hints)
{
	struct cli_print *data = hints;
	struct cli_data *cli;

	cli = st_data;
	if (!cli->func)
		return;

	if (cli->help)
		data->print(data->hints, "%-10s %s\n", cmd_name, cli->help);
	else
		data->print(data->hints, "%-10s No help text available\n",
				cmd_name);
}

void cli_print_help(struct st_root *root, void (*print)(void*, const char*, ...),
		void *hints)
{
	struct cli_print data;
	data.print = print;
	data.hints = hints;

	st_foreach_data(root, __cli_print_help, &data);
}
