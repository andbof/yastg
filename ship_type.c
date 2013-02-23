#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "port.h"
#include "common.h"
#include "stringtree.h"
#include "log.h"
#include "parseconfig.h"
#include "ptrlist.h"
#include "ship_type.h"
#include "universe.h"

static void ship_type_init(struct ship_type * const type)
{
	memset(type, 0, sizeof(*type));
	INIT_LIST_HEAD(&type->list);
}

void ship_type_free(struct ship_type * const type)
{
	free(type->name);
	free(type->desc);
}

static int set_description(struct ship_type *type, struct config *conf)
{
	if (!conf->str)
		return -1;

	type->desc = strdup(conf->str);
	if (!type->desc)
		return -1;

	return 0;
}

static int set_carry_weight(struct ship_type *type, struct config *conf)
{
	type->carry_weight = conf->l;
	return 0;
}

static int build_command_tree(struct list_head *root)
{
	if (st_add_string(root, "carryweight", set_carry_weight))
		return -1;
	if (st_add_string(root, "description", set_description))
		return -1;

	return 0;
}

int load_ships_from_file(const char * const file, struct universe * const universe)
{
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	struct config *conf, *child;
	struct ship_type *sh_type;
	int (*func)(struct ship_type*, struct config*);
	assert(file);

	if (build_command_tree(&cmd_root))
		return -1;

	if (parse_config_file(file, &conf_root))
		return -1;

	list_for_each_entry(conf, &conf_root, list) {
		sh_type = malloc(sizeof(*sh_type));
		if (!sh_type)
			goto err;

		ship_type_init(sh_type);
		sh_type->name = strdup(conf->key);
		if (!sh_type->name) {
			free(sh_type);
			goto err;
		}

		list_for_each_entry(child, &conf->children, list) {
			func = st_lookup_string(&cmd_root, child->key);
			if (!func) {
				log_printfn(LOG_CONFIG, "unknown ship type key: \"%s\"", child->key);
				continue;
			}

			if (func(sh_type, child)) {
				log_printfn(LOG_CONFIG, "syntax error or out of memory when processing ship type key \"%s\"", child->key);
				continue;
			}
		}

		if (st_add_string(&universe->ship_type_names, sh_type->name, sh_type)) {
			ship_type_free(sh_type);
			free(sh_type);
			goto err;
		}
		list_add_tail(&sh_type->list, &universe->ship_types);
	}

	destroy_config(&conf_root);
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);

	return 0;

err:
	destroy_config(&conf_root);
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return -1;
}
