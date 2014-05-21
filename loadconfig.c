#include <alloca.h>
#include <assert.h>
#include <basedir.h>
#include <basedir_fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "loadconfig.h"
#include "log.h"
#include "item.h"
#include "names.h"
#include "planet_type.h"
#include "port_type.h"
#include "ship_type.h"
#include "star.h"
#include "stringtrie.h"

struct file_list {
	char *name;
	struct list_head list;
};

struct config_type {
	const char key[16];
	struct list_head head;
	int (*func)(const char * const file, struct universe * const);
};

static xdgHandle xdg_handle;

#define CONFIG_PREFIX "yastg/"
static char *get_conf_file(const char * const name)
{
	char *path, *file;

	if (!name || !*name)
		return NULL;

	path = malloc(sizeof(CONFIG_PREFIX) + strlen(name) + 1);
	if (!path)
		goto mem_err;
	memcpy(path, CONFIG_PREFIX, sizeof(CONFIG_PREFIX));
	strcat(path, name);

	file = xdgConfigFind(path, &xdg_handle);
	if (!*file)
		goto file_err;

	free(path);
	return file;

file_err:
	log_printfn(LOG_CONFIG, "file not found: \"%s\"\n", path);
	free(path);
mem_err:
	return NULL;
}

static int build_list_of_file_names(struct config_type configs[], const size_t len, const struct list_head *conf_root)
{
	struct st_root cmd_root;
	st_init(&cmd_root);

	for (size_t i = 0; i < len; i++) {
		if (st_add_string(&cmd_root, configs[i].key, &configs[i].head))
			goto err;
	}

	struct config *conf;
	list_for_each_entry(conf, conf_root, list) {
		struct list_head *head = st_lookup_string(&cmd_root, conf->key);
		if (!head) {
			log_printfn(LOG_CONFIG, "invalid keyword \"%s\"", conf->key);
			continue;
		}

		if (!conf->str) {
			log_printfn(LOG_CONFIG, "keyword \"%s\" is empty, ignored", conf->key);
			continue;
		}

		char *file = get_conf_file(conf->str);
		if (!file) {
			log_printfn(LOG_CONFIG, "cannot find configuration file \"%s\"", conf->str);
			continue;
		}

		struct file_list *list = malloc(sizeof(*list));
		if (!list)
			goto err;
		list->name = file;
		list_add_tail(&list->list, head);
	}

	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return 0;

err:
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return -1;
}

static int do_load_from_files(const struct config_type configs[], const size_t len,
		struct universe * const universe)
{
	struct file_list *f;
	int r;

	log_printfn(LOG_CONFIG, "the following configuration files will be loaded");
	for (size_t i = 0; i < len; i++) {
		list_for_each_entry(f, &configs[i].head, list)
			log_printfn(LOG_CONFIG, "%s: %s ", configs[i].key, f->name);

		list_for_each_entry(f, &configs[i].head, list) {
			if (configs[i].func) {
				r = configs[i].func(f->name, universe);
				if (r)
					return r;
			}
		}
	}

	return 0;
}

/*
 * Names are a special case and can't be treated like all other files,
 * as we need to pass multiple file names to load_all_names(), and exactly
 * which ones depend on the keys. That's why they are handled in a separate
 * function and not in do_load_from_files() above.
 */
static int load_names_from_files(const struct config_type configs[], const size_t len)
{
	struct st_root cmd_root;
	const char *constellations = NULL;
	const char *first = NULL;
	const char *sur = NULL;
	const char *place = NULL;
	const char *prefix = NULL;
	const char *suffix = NULL;
	struct key_val {
		char *key;
		const char **val;
	};

	struct key_val key_vals[] = {
		{ .key = "constellations",	.val = &constellations },
		{ .key = "firstnames",		.val = &first, },
		{ .key = "surnames",		.val = &sur, },
		{ .key = "placenames",		.val = &place },
		{ .key = "placeprefix",		.val = &prefix },
		{ .key = "placesuffix", 	.val = &suffix },
	};

	st_init(&cmd_root);

	for (size_t i = 0; i < ARRAY_SIZE(key_vals); i++) {
		if (st_add_string(&cmd_root, key_vals[i].key, key_vals[i].val))
			goto err;
	}

	const char **s;
	for (size_t i = 0; i < len; i++) {
		s = st_lookup_string(&cmd_root, configs[i].key);
		if (!s)
			continue;

		assert(!list_empty(&configs[i].head));
		*s = list_first_entry(&configs[i].head, struct file_list, list)->name;
		assert(*s);
	}

	if (constellations)
		names_load(&univ.avail_constellations, NULL, constellations, NULL, NULL);

	if (first || sur) {
		if (first && sur)
			names_load(&univ.avail_player_names, NULL, first, sur, NULL);
		else
			log_printfn(LOG_CONFIG, "need both first names and surnames");
	}

	if (place || prefix || suffix) {
		if (place && prefix && suffix)
			names_load(&univ.avail_port_names, prefix, place, NULL, suffix);
		else
			log_printfn(LOG_CONFIG, "need all of placenames, placeprefix and placesuffix");
	}

	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return 0;
err:
	st_destroy(&cmd_root, ST_DONT_FREE_DATA);
	return -1;
}

static int load_config(struct universe * const universe, struct list_head * const config_root)
{
	struct file_list *f, *_f;
	int r = 0;

	/*
	 * The files will be loaded in the order they are listed here.
	 * It is very important this is correct, as some of them (e.g. items)
	 * must be loaded before others (e.g. planets).
	 */
	struct config_type configs[] = {
		{ .key = "civilizations",	.func = load_civs_from_file, },
		{ .key = "constellations",	.func = NULL, },
		{ .key = "firstnames",		.func = NULL, },
		{ .key = "surnames",		.func = NULL, },
		{ .key = "placenames",		.func = NULL, },
		{ .key = "placeprefix",		.func = NULL, },
		{ .key = "placesuffix",		.func = NULL, },
		{ .key = "items",		.func = load_items_from_file, },
		{ .key = "ships",		.func = load_ships_from_file, },
		{ .key = "ports",		.func = load_ports_from_file, },
		{ .key = "planets",		.func = load_planets_from_file, },
	};

	for (size_t i = 0; i < ARRAY_SIZE(configs); i++)
		INIT_LIST_HEAD(&configs[i].head);

	r = build_list_of_file_names(configs, ARRAY_SIZE(configs), config_root);
	if (r)
		goto cleanup;

	r = do_load_from_files(configs, ARRAY_SIZE(configs), universe);
	if (r)
		goto cleanup;

	r = load_names_from_files(configs, ARRAY_SIZE(configs));
	if (r)
		goto cleanup;

cleanup:
	for (size_t i = 0; i < ARRAY_SIZE(configs); i++) {
		list_for_each_entry_safe(f, _f, &configs[i].head, list) {
			list_del(&f->list);
			free(f->name);
			free(f);
		}
	}
	return r;
}

static int is_config_sane(struct universe * const universe)
{
	int r = 1;

	if (list_empty(&universe->items)) {
		log_printfn(LOG_CONFIG, "error: no items loaded");
		r = 0;
	} else {
		assert(!st_is_empty(&universe->item_names));
	}

	if (list_empty(&universe->port_types)) {
		log_printfn(LOG_CONFIG, "error: no ports loaded");
		r = 0;
	} else {
		assert(!st_is_empty(&universe->port_type_names));
	}

	if (list_empty(&universe->planet_types)) {
		log_printfn(LOG_CONFIG, "error: no planet types loaded");
		r = 0;
	}

	if (list_empty(&universe->ship_types)) {
		log_printfn(LOG_CONFIG, "error: no ship types loaded");
		r = 0;
	} else {
		assert(!st_is_empty(&universe->ship_type_names));
	}

	if (list_empty(&universe->civs)) {
		log_printfn(LOG_CONFIG, "error: no civilizations loaded");
		r = 0;
	}

	if (!is_names_loaded(&universe->avail_constellations)) {
		log_printfn(LOG_CONFIG, "error: no constellations loaded");
		r = 0;
	}

	if (!is_names_loaded(&universe->avail_port_names)) {
		log_printfn(LOG_CONFIG, "error: no port names loaded");
		r = 0;
	}

	if (!is_names_loaded(&universe->avail_player_names)) {
		log_printfn(LOG_CONFIG, "error: no player names loaded");
		r = 0;
	}

	return r;
}

#define CONFIG_FILE "yastg/yastg.conf"
int parse_config_files(struct universe * const universe)
{
	struct list_head conf = LIST_HEAD_INIT(conf);
	char *c;
	printf("Parsing configuration files\n");
	xdgInitHandle(&xdg_handle);

	c = xdgConfigFind(CONFIG_FILE, &xdg_handle);
	if (!c) {
		log_printfn(LOG_CONFIG, CONFIG_FILE "not found\n");
		return -1;
	}

	log_printfn(LOG_CONFIG, "parsing \"%s\"\n", c);
	if (parse_config_file(c, &conf)) {
		destroy_config(&conf);
		return -1;
	}
	if (load_config(universe, &conf)) {
		destroy_config(&conf);
		return -1;
	}
	destroy_config(&conf);
	free(c);
	xdgWipeHandle(&xdg_handle);

	if (!is_config_sane(universe))
		return -1;

	return 0;
}
