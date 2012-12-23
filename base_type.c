#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "base_type.h"
#include "base.h"
#include "item.h"
#include "stringtree.h"
#include "log.h"
#include "parseconfig.h"
#include "universe.h"

char base_zone_names[BASE_ZONE_NUM][8] = {
	"oceanic",
	"surface",
	"orbit",
	"rogue",
};

static void base_type_item_init(struct base_type_item *item)
{
	memset(item, 0, sizeof(*item));
	ptrlist_init(&item->requires);
}

static void base_type_init(struct base_type *type)
{
	memset(type, 0, sizeof(*type));
	INIT_LIST_HEAD(&type->list);
	INIT_LIST_HEAD(&type->items);
}

void base_type_free(struct base_type *type)
{
	free(type->name);
	free(type->desc);

	struct base_type_item *i, *_i;
	list_for_each_entry_safe(i, _i, &type->items, list) {
		list_del(&i->list);
		free(i);
	}
}

static int set_description(struct base_type *type, struct config *conf)
{
	if (conf->str)
		type->desc = strdup(conf->str);
	else
		type->desc = strdup("");

	return 0;
}

static int add_zone(struct base_type *type, struct config *conf)
{
	struct config *child;
	int i;
	struct list_head zone_root = LIST_HEAD_INIT(zone_root);
	int zones[BASE_ZONE_NUM];

	for (i = 0; i < BASE_ZONE_NUM; i++)
		zones[i] = i;

	for (i = 0; i < BASE_ZONE_NUM; i++)
		if (st_add_string(&zone_root, base_zone_names[i], &zones[i]))
			return -1;

	list_for_each_entry(child, &conf->children, list) {
		int *j = st_lookup_string(&zone_root, child->key);
		if (!j)
			return -1;

		type->zones[*j] = 1;
	}

	st_destroy(&zone_root, ST_DONT_FREE_DATA);

	return 0;
}

static int set_item_capacity(struct base_type_item *item, struct config *conf)
{
	item->capacity = conf->l;
	return 0;
}

static int set_item_produces(struct base_type_item *item, struct config *conf)
{
	item->daily_change += conf->l;
	return 0;
}

static int set_item_consumes(struct base_type_item *item, struct config *conf)
{
	item->daily_change -= conf->l;
	return 0;
}

static int set_item_requires(struct base_type_item *item, struct config *conf)
{
	struct item *req = st_lookup_string(&univ.item_names, conf->str);

	if (!req) {
		log_printfn("config", "unknown item '%s'", conf->str);
		return -1;
	}

	ptrlist_push(&item->requires, req);
	return 0;
}

static int build_item_cmdtree(struct list_head *root)
{
	if (st_add_string(root, "capacity", set_item_capacity))
		return -1;
	if (st_add_string(root, "produces", set_item_produces))
		return -1;
	if (st_add_string(root, "consumes", set_item_consumes))
		return -1;
	if (st_add_string(root, "requires", set_item_requires))
		return -1;

	return 0;
}

static int add_item(struct base_type *type, struct config *conf)
{
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	struct item *item;
	if (build_item_cmdtree(&cmd_root))
		return -1;

	if (!conf->str) {
		log_printfn("config", "syntax error after \"item\"");
		goto err;
	}

	item = st_lookup_string(&univ.item_names, conf->str);
	if (!item) {
		log_printfn("config", "unknown item: \"%s\"", conf->str);
		goto err;
	}

	struct base_type_item *type_item = malloc(sizeof(*type_item));
	if (!type_item)
		goto err;

	base_type_item_init(type_item);
	type_item->item = item;

	int (*func)(struct base_type_item*, struct config*);
	struct config *child;
	list_for_each_entry(child, &conf->children, list) {
		func = st_lookup_string(&cmd_root, child->key);
		if (!func) {
			log_printfn("config", "unknown base type item key: \"%s\"\n", child->key);
			continue;
		}

		if (func(type_item, child))
			continue;
	}

	list_add(&type_item->list, &type->items);

	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return 0;

err:
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return -1;
}

static int build_command_tree(struct list_head *root)
{
	if (st_add_string(root, "description", set_description))
		return -1;
	if (st_add_string(root, "zones", add_zone))
		return -1;
	if (st_add_string(root, "item", add_item))
		return -1;

	return 0;
}

int load_all_bases(struct list_head * const root)
{
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	struct config *conf, *child;
	struct base_type *type;
	void (*func)(struct base_type*, struct config*);

	if (build_command_tree(&cmd_root))
		return -1;

	if (parse_config_file("data/bases", &conf_root))
		return -1;

	list_for_each_entry(conf, &conf_root, list) {
		type = malloc(sizeof(*type));
		if (!type)
			goto err;
		base_type_init(type);

		type->name = strdup(conf->key);
		if (!type->name) {
			free(type);
			goto err;
		}

		list_for_each_entry(child, &conf->children, list) {
			func = st_lookup_string(&cmd_root, child->key);
			if (!func) {
				log_printfn("config", "unknown base type key: \"%s\"\n", child->key);
				continue;
			}

			func(type, child);
		}

		if (st_add_string(&univ.base_type_names, type->name, type)) {
			base_type_free(type);
			free(type);
			goto err;
		}

		list_add(&type->list, root);
	}

	destroy_config(&conf_root);
	st_destroy(&cmd_root, 0);
	return 0;

err:
	destroy_config(root);
	st_destroy(&cmd_root, 0);
	return -1;
}
