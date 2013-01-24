#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "port.h"
#include "common.h"
#include "stringtree.h"
#include "log.h"
#include "parseconfig.h"
#include "ptrlist.h"
#include "planet_type.h"
#include "universe.h"

const char planet_life_desc[PLANET_LIFE_NUM][PLANET_LIFE_DESC_LEN] = {
	"Toxic to life",
	"Barren, no life",
	"Dead planet",
	"Single-cellular life",
	"Bacterial life",
	"Simple lifeforms",
	"Resistant plantlife",
	"Complex plantlife",
	"Animal life",
	"Intelligent humanoid life"
};

char planet_life_names[PLANET_LIFE_NUM][PLANET_LIFE_NAME_LEN] = {
	"TOXIC",
	"BARREN",
	"DEAD",
	"SINGLECELL",
	"BACTERIA",
	"SIMPLE",
	"RESISTANT",
	"COMPLEX",
	"ANIMAL",
	"INTELLIGENT",
};

char planet_zone_names[PLANET_ZONE_NUM][PLANET_ZONE_NAME_LEN] = {
	"HOT",
	"ECO",
	"COLD",
};

static void planet_type_init(struct planet_type * const type)
{
	memset(type, 0, sizeof(*type));
	ptrlist_init(&type->base_types);
	INIT_LIST_HEAD(&type->list);
}

void planet_type_free(struct planet_type * const type)
{
	free(type->name);
	free(type->desc);
	free(type->surface);
	free(type->atmo);
	ptrlist_free(&type->base_types);
}

static int set_atmosphere(struct planet_type *type, struct config *conf)
{
	if (!conf->str)
		return -1;

	type->atmo = strdup(conf->str);
	if (!type->atmo)
		return -1;

	return 0;
}

static int set_base_types(struct planet_type *type, struct config *conf)
{
	struct config *child;
	struct base_type *base;

	list_for_each_entry(child, &conf->children, list) {
		base = st_lookup_string(&univ.base_type_names, child->key);
		if (!base)
			return -1;

		ptrlist_push(&type->base_types, base);
	}

	return 0;
}

static int set_description(struct planet_type *type, struct config *conf)
{
	if (!conf->str)
		return -1;

	type->desc = strdup(conf->str);
	if (!type->desc)
		return -1;

	return 0;
}

static int set_life(enum planet_life *life, char *value)
{
	int i;
	struct list_head life_root = LIST_HEAD_INIT(life_root);
	int lifes[PLANET_LIFE_NUM];

	if (!value)
		return -1;

	for (i = 0; i < ARRAY_SIZE(lifes); i++)
		lifes[i] = i;

	for (i = 0; i < ARRAY_SIZE(lifes); i++)
		if (st_add_string(&life_root, planet_life_names[i], &lifes[i]))
			return -1;

	int *j = st_lookup_string(&life_root, value);
	if (!j)
		return -1;

	*life = *j;

	st_destroy(&life_root, ST_DONT_FREE_DATA);

	return 0;
}

static int set_maximum_diameter(struct planet_type *type, struct config *conf)
{
	type->maxdia = limit_long_to_uint(conf->l);
	return 0;
}

static int set_maximum_life(struct planet_type *type, struct config *conf)
{
	return set_life(&type->maxlife, conf->str);
}

static int set_minimum_diameter(struct planet_type *type, struct config *conf)
{
	type->mindia = limit_long_to_uint(conf->l);
	return 0;
}

static int set_minimum_life(struct planet_type *type, struct config *conf)
{
	return set_life(&type->minlife, conf->str);
}

static int set_name(struct planet_type *type, struct config *conf)
{
	if (!conf->str)
		return -1;

	type->name = strdup(conf->str);
	if (!type->name)
		return -1;

	return 0;
}

static int set_surface(struct planet_type *type, struct config *conf)
{
	if (!conf->str)
		return -1;

	type->surface = strdup(conf->str);
	if (!type->surface)
		return -1;

	return 0;
}

static int set_zones(struct planet_type *type, struct config *conf)
{
	struct config *child;
	unsigned int i;
	struct list_head zone_root = LIST_HEAD_INIT(zone_root);
	int zones[PLANET_ZONE_NUM];

	for (i = 0; i < ARRAY_SIZE(zones); i++)
		zones[i] = i;

	for (i = 0; i < ARRAY_SIZE(zones); i++)
		if (st_add_string(&zone_root, planet_zone_names[i], &zones[i]))
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

static int build_command_tree(struct list_head *root)
{
	if (st_add_string(root, "atmosphere", set_atmosphere))
		return -1;
	if (st_add_string(root, "bases", set_base_types))
		return -1;
	if (st_add_string(root, "description", set_description))
		return -1;
	if (st_add_string(root, "maxdiameter", set_maximum_diameter))
		return -1;
	if (st_add_string(root, "maxlife", set_maximum_life))
		return -1;
	if (st_add_string(root, "mindiameter", set_minimum_diameter))
		return -1;
	if (st_add_string(root, "minlife", set_minimum_life))
		return -1;
	if (st_add_string(root, "name", set_name))
		return -1;
	if (st_add_string(root, "surface", set_surface))
		return -1;
	if (st_add_string(root, "zones", set_zones))
		return -1;

	return 0;
}

int load_all_planets(struct list_head * const root)
{
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	struct list_head cmd_root = LIST_HEAD_INIT(cmd_root);
	struct config *conf, *child;
	struct planet_type *pl_type;
	int (*func)(struct planet_type*, struct config*);

	if (build_command_tree(&cmd_root))
		return -1;

	if (parse_config_file("data/planets", &conf_root))
		return -1;

	list_for_each_entry(conf, &conf_root, list) {
		pl_type = malloc(sizeof(*pl_type));
		if (!pl_type)
			goto err;

		planet_type_init(pl_type);
		pl_type->c = conf->key[0];

		list_for_each_entry(child, &conf->children, list) {
			func = st_lookup_string(&cmd_root, child->key);
			if (!func) {
				log_printfn("config", "unknown planet type key: \"%s\"", child->key);
				continue;
			}

			if (func(pl_type, child)) {
				log_printfn("config", "syntax error or out of memory when processing planet type key \"%s\"", child->key);
				continue;
			}
		}

		list_add_tail(&pl_type->list, root);
	}

	destroy_config(&conf_root);
	st_destroy(&cmd_root, 0);
	return 0;

err:
	destroy_config(&conf_root);
	st_destroy(&cmd_root, 0);
	return -1;
}
