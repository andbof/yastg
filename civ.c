#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "common.h"
#include "log.h"
#include "mtrandom.h"
#include "civ.h"
#include "ptrlist.h"
#include "system.h"
#include "parseconfig.h"
#include "universe.h"
#include "list.h"

#define CIV_MIN_BORDER_WIDTH (200 * TICK_PER_LY)
static int is_border_system(struct system *system, struct civ *c)
{
	struct ptrlist neigh;
	struct list_head *lh;
	struct system *s;
	int is_border = 0;

	ptrlist_init(&neigh);

	get_neighbouring_systems(&neigh, system, CIV_MIN_BORDER_WIDTH);
	ptrlist_for_each_entry(s, &neigh, lh) {
		if (!s->owner) {
			is_border = 1;
		} else if (s->owner && s->owner != c) {
			is_border = 0;
			break;
		}
	}

	ptrlist_free(&neigh);

	return is_border;
}

#define CIV_GROW_MIN_LY (10 * TICK_PER_LY)
#define CIV_GROW_STEP_LY (10 * TICK_PER_LY)
static int grow_civ(struct universe *u, struct civ *c)
{
	struct system *s, *t;
	struct ptrlist neigh;
	struct list_head *lh;
	unsigned long radius;

	if (ptrlist_len(&c->border_systems) == 0)
		return 1;

	t = ptrlist_entry(&c->border_systems, 0);
	radius = CIV_GROW_MIN_LY;

	do {
		ptrlist_init(&neigh);
		s = NULL;

		get_neighbouring_systems(&neigh, t, radius);
		ptrlist_for_each_entry(s, &neigh, lh) {
			if (!s->owner)
				break;
		}
		if ((s == NULL) || (s->owner))
			radius += CIV_GROW_STEP_LY;

		ptrlist_free(&neigh);
	} while ((s == NULL) || (s->owner));

	s->owner = c;
	linksystems(s, t);
	ptrlist_push(&c->systems, s);

	if (is_border_system(s, c))
		ptrlist_push(&c->border_systems, s);
	if (!is_border_system(t, c))
		ptrlist_rm(&c->border_systems, 0);

	printf("Growing civ %s into %s at %ldx%ld\n", c->name, s->name, s->x, s->y);

	return 0;
}

#define INITIAL_MIN_INTERCIV_DISTANCE_LY (10 * CIV_MIN_BORDER_WIDTH)
static void spawn_civilizations(struct universe *u, struct civ *civs)
{
	struct civ *c;
	struct system *s;
	int success, tries;
	struct ptrlist neigh;
	struct list_head *lh;

	list_for_each_entry(c, &civs->list, list) {
		tries = 0;
		do {
			tries++;
			success = 1;
			s = ptrlist_random(&u->systems);
			if (!s->owner) {
				ptrlist_init(&neigh);

				get_neighbouring_systems(&neigh, s, INITIAL_MIN_INTERCIV_DISTANCE_LY);
				struct system *t;
				ptrlist_for_each_entry(t, &neigh, lh) {
					if (t->owner != 0) {
						success = 0;
						break;
					}
				}

				ptrlist_free(&neigh);
			} else {
				success = 0;
			}
		} while (!success && tries < 100);

		if (tries >= 100)
			break;

		mprintf("Chose %s as home system for %s\n", s->name, c->name);
		s->owner = c;
		c->home = s;
		ptrlist_push(&c->systems, s);
		ptrlist_push(&c->border_systems, s);
		univ.inhabited_systems++;
	}
}

static void remove_civs_without_homes(struct universe *u, struct civ *civs)
{
	struct civ *c, *_c;

	list_for_each_entry_safe(c, _c, &civs->list, list) {
		if (c->home)
			continue;

		list_del_init(&c->list);
		civ_free(c);
		free(c);
	}
}

#define UNIVERSE_CIV_FRAC 0.4
static void grow_all_civs(struct universe *u, struct civ *civs)
{
	unsigned long goal_hab, total_power;
	struct civ *c, *_c;

	mprintf("Growing civilizations ...\n");

	goal_hab = ptrlist_len(&u->systems) * UNIVERSE_CIV_FRAC;

	total_power = 0;
	list_for_each_entry(c, &civs->list, list) {
		total_power += c->power;
	}

	struct list_head growing_civs = LIST_HEAD_INIT(growing_civs);
	list_for_each_entry(c, &civs->list, list)
		list_add(&c->growing, &growing_civs);

	while (univ.inhabited_systems < goal_hab && !list_empty(&growing_civs)) {
		list_for_each_entry_safe(c, _c, &growing_civs, growing) {
			if (mtrandom_ulong(total_power) >= c->power)
				continue;

			if (!grow_civ(u, c))
				univ.inhabited_systems++;
			else
				list_del(&c->growing);
		}
	}

	mprintf("done.\n");
}

void civ_spawncivs(struct universe *u, struct civ *civs)
{
	struct civ *c;

	spawn_civilizations(u, civs);

	remove_civs_without_homes(u, civs);

	grow_all_civs(u, civs);

	mprintf("Civilization stats:\n");
	list_for_each_entry(c, &civs->list, list)
		mprintf("  %s has %lu systems (%.2f%%) with power %u\n", c->name, ptrlist_len(&c->systems), ptrlist_len(&c->systems)/(float)univ.inhabited_systems*100, c->power);
	mprintf("%lu systems of %lu are inhabited (%.2f%%)\n", univ.inhabited_systems, ptrlist_len(&u->systems), univ.inhabited_systems/(float)ptrlist_len(&u->systems)*100);
}

void civ_init(struct civ *c)
{
	memset(c, 0, sizeof(*c));
	ptrlist_init(&c->presystems);
	ptrlist_init(&c->availnames);
	ptrlist_init(&c->systems);
	ptrlist_init(&c->border_systems);
	INIT_LIST_HEAD(&c->list);
	INIT_LIST_HEAD(&c->growing);
}

void loadciv(struct civ *c, const struct list_head * const config_root)
{
	struct config *conf;
	char *st;
	civ_init(c);

	list_for_each_entry(conf, config_root, list) {
		if (strcmp(conf->key, "NAME") == 0) {
			c->name = strdup(conf->str);
		} else if (strcmp(conf->key, "HOME") == 0) {
		} else if (strcmp(conf->key, "POWER") == 0) {
			c->power = limit_long_to_int(conf->l);
		} else if (strcmp(conf->key, "SYSTEM") == 0) {
			printf("FIXME: SYSTEM is not supported\n");
		} else if (strcmp(conf->key, "SNAME") == 0) {
			st = strdup(conf->str);
			if (st)
				ptrlist_push(&c->availnames, st);
		}
	}
}

int civ_load_all(struct civ *civs)
{
	DIR *dirp;
	struct dirent *de;
	struct civ *cv;
	struct list_head conf = LIST_HEAD_INIT(conf);
	char* path = NULL;
	int pathlen = 0;
	int r = 0;

	if (!(dirp = opendir("civs")))
		die("%s", "opendir() on civs failed");

	while ((de = readdir(dirp)) != NULL) {
		if (de->d_name[0] != '.') {
			if ((int)strlen(de->d_name) > pathlen-6) {
				free(path);
				pathlen = strlen(de->d_name)+6;
				path = malloc(pathlen);
			}

			sprintf(path, "%s/%s", "civs", de->d_name);
			if (parse_config_file(path, &conf)) {
				destroy_config(&conf);
				r = -1;
				goto close;
			}

			cv = malloc(sizeof(*cv));
			if (!cv) {
				r = -1;
				goto close;
			}

			loadciv(cv, &conf);
			destroy_config(&conf);
			list_add_tail(&(cv->list), &(civs->list));
		}
	}

close:
	free(path);
	closedir(dirp);

	return r;
}

void civ_free(struct civ *civ)
{
	char *c;
	struct list_head *lh;
	ptrlist_free(&civ->systems);
	ptrlist_free(&civ->border_systems);
	ptrlist_free(&civ->presystems);
	ptrlist_for_each_entry(c, &civ->availnames, lh)
		free(c);
	ptrlist_free(&civ->availnames);
	free(civ->name);
}
