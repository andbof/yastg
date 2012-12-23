#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "item.h"
#include "log.h"
#include "list.h"
#include "parseconfig.h"
#include "stringtree.h"

static void set_weight(struct item *item, struct config *conf)
{
	item->weight = conf->l;
}

static void build_command_tree(struct list_head *root)
{
	st_add_string(root, "weight", set_weight);
}

int load_all_items(struct list_head * const root, struct list_head * const item_tree)
{
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	struct config *conf, *child;
	struct item *item;
	void (*func)(struct item*, struct config*);

	build_command_tree(&cmd_root);

	if (parse_config_file("data/items", &conf_root))
		return -1;

	list_for_each_entry(conf, &conf_root, list) {
		item = malloc(sizeof(*item));
		if (!item)
			goto err;

		item->name = strdup(conf->key);
		if (!item->name) {
			free(item);
			goto err;
		}

		if (st_add_string(item_tree, item->name, item)) {
			free(item->name);
			free(item);
			goto err;
		}

		list_for_each_entry(child, &conf->children, list) {
			func = st_lookup_string(&cmd_root, child->key);
			if (!func) {
				log_printfn("config", "unknown item key: \"%s\"", child->key);
				goto err;
			}

			func(item, child);
		}

		list_add(&item->list, root);
	}

	destroy_config(&conf_root);
	st_destroy(&cmd_root, 0);
	return 0;

err:
	destroy_config(&conf_root);
	st_destroy(&cmd_root, 0);
	return -1;
}

void item_free(struct item * const item)
{
	free(item->name);
}
