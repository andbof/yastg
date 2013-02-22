#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "port_type.h"
#include "port.h"
#include "cargo.h"
#include "item.h"
#include "stringtree.h"
#include "log.h"
#include "parseconfig.h"
#include "universe.h"

char port_zone_names[PORT_ZONE_NUM][8] = {
	"oceanic",
	"surface",
	"orbit",
	"rogue",
};

static void port_type_init(struct port_type *type)
{
	memset(type, 0, sizeof(*type));
	INIT_LIST_HEAD(&type->list);
	INIT_LIST_HEAD(&type->items);
	INIT_LIST_HEAD(&type->item_names);
}

void port_type_free(struct port_type *type)
{
	free(type->name);
	free(type->desc);

	struct cargo *c, *_c;
	list_for_each_entry_safe(c, _c, &type->items, list) {
		list_del(&c->list);
		cargo_free(c),
		free(c);
	}

	st_destroy(&type->item_names, ST_DONT_FREE_DATA);
}

static int set_description(struct port_type *type, struct config *conf)
{
	if (conf->str)
		type->desc = strdup(conf->str);
	else
		type->desc = strdup("");

	return 0;
}

static int add_zone(struct port_type *type, struct config *conf)
{
	struct config *child;
	int i;
	struct list_head zone_root = LIST_HEAD_INIT(zone_root);
	int zones[PORT_ZONE_NUM];

	for (i = 0; i < PORT_ZONE_NUM; i++)
		zones[i] = i;

	for (i = 0; i < PORT_ZONE_NUM; i++)
		if (st_add_string(&zone_root, port_zone_names[i], &zones[i]))
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

static int set_item_capacity(struct cargo *cargo, struct list_head *item_names, struct config *conf)
{
	cargo->max = conf->l;
	return 0;
}

static int set_item_produces(struct cargo *cargo, struct list_head *item_names, struct config *conf)
{
	cargo->daily_change += conf->l;
	return 0;
}

static int set_item_consumes(struct cargo *cargo, struct list_head *item_names, struct config *conf)
{
	cargo->daily_change -= conf->l;
	return 0;
}

static int set_item_requires(struct cargo *cargo, struct list_head *item_names, struct config *conf)
{
	struct cargo *req = st_lookup_string(item_names, conf->str);

	if (!req) {
		log_printfn(LOG_CONFIG, "item '%s' does not exist in this port", conf->str);
		return -1;
	}

	ptrlist_push(&cargo->requires, req);
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

static int add_item(struct port_type *type, struct config *conf)
{
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	if (build_item_cmdtree(&cmd_root))
		goto err;
	if (!conf->str)
		goto err;

	struct cargo *cargo = st_lookup_string(&type->item_names, conf->str);
	if (!cargo)
		goto err;

	int (*func)(struct cargo*, struct list_head *item_names, struct config*);
	struct config *child;
	list_for_each_entry(child, &conf->children, list) {
		func = st_lookup_string(&cmd_root, child->key);
		if (!func) {
			log_printfn(LOG_CONFIG, "unknown port type item key: \"%s\"\n", child->key);
			continue;
		}

		if (func(cargo, &type->item_names, child))
			continue;
	}

	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return 0;

err:
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return -1;
}

static int pre_add_item(struct port_type *type, struct config *conf)
{
	struct cargo *cargo = NULL;

	if (!conf->str) {
		log_printfn(LOG_CONFIG, "syntax error after \"item\"");
		goto err;
	}

	struct item *item = st_lookup_string(&univ.item_names, conf->str);
	if (!item) {
		log_printfn(LOG_CONFIG, "unknown item: \"%s\"", conf->str);
		goto err;
	}

	cargo = malloc(sizeof(*cargo));
	if (!cargo)
		goto err;
	cargo_init(cargo);

	cargo->item = item;
	if (st_add_string(&type->item_names, item->name, cargo))
		goto err;

	list_add(&cargo->list, &type->items);
	return 0;

err:
	if (cargo)
		cargo_free(cargo);
	free(cargo);
	return -1;
}

static int build_command_tree(struct list_head *root)
{
	if (st_add_string(root, "description", set_description))
		return -1;
	if (st_add_string(root, "zones", add_zone))
		return -1;
	if (st_add_string(root, "item", pre_add_item))
		return -1;

	return 0;
}

int load_all_ports()
{
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	struct list_head item_root = LIST_HEAD_INIT(item_root);
	struct config *conf, *child;
	struct port_type *type;
	int (*func)(struct port_type*, struct config*);

	if (build_command_tree(&cmd_root))
		goto err;
	if (st_add_string(&item_root, "item", add_item))
		goto err;

	if (parse_config_file("data/ports", &conf_root))
		goto err;

	list_for_each_entry(conf, &conf_root, list) {
		type = malloc(sizeof(*type));
		if (!type)
			goto err;
		port_type_init(type);

		type->name = strdup(conf->key);
		if (!type->name) {
			free(type);
			goto err;
		}

		/*
		 * We need to parse in two steps because parsing the items requires
		 * all items to exist (as they have mutual dependencies). The first
		 * step allocates the items and builds the stringtree, while the
		 * second step does the actual item parsing.
		 */
		list_for_each_entry(child, &conf->children, list) {
			func = st_lookup_string(&cmd_root, child->key);
			if (!func) {
				log_printfn(LOG_CONFIG, "unknown port type key: \"%s\"\n", child->key);
				continue;
			}

			if (func(type, child))
				goto err;
		}

		list_for_each_entry(child, &conf->children, list) {
			func = st_lookup_string(&item_root, child->key);
			if (func && func(type, child))
				goto err;
		}

		if (st_add_string(&univ.port_type_names, type->name, type)) {
			port_type_free(type);
			free(type);
			goto err;
		}

		list_add(&type->list, &univ.port_types);
	}

	destroy_config(&conf_root);
	st_destroy(&item_root, ST_DONT_FREE_DATA);
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return 0;

err:
	destroy_config(&univ.port_types);
	st_destroy(&item_root, ST_DONT_FREE_DATA);
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return -1;
}
