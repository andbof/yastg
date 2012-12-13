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
#include "sector.h"
#include "parseconfig.h"
#include "universe.h"
#include "list.h"

#define CIV_MIN_BORDER_WIDTH (200 * TICK_PER_LY)
static int is_border_sector(struct sector *sector, struct civ *c)
{
	struct ptrlist neigh;
	struct list_head *lh;
	struct sector *s;
	int is_border = 0;

	ptrlist_init(&neigh);

	get_neighbouring_systems(&neigh, sector, CIV_MIN_BORDER_WIDTH);
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
	struct sector *s, *t;
	struct ptrlist neigh;
	struct list_head *lh;
	unsigned long radius;

	if (ptrlist_len(&c->border_sectors) == 0)
		return 1;

	t = ptrlist_entry(&c->border_sectors, 0);
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
	linksectors(s, t);
	ptrlist_push(&c->sectors, s);

	if (is_border_sector(s, c))
		ptrlist_push(&c->border_sectors, s);
	if (!is_border_sector(t, c))
		ptrlist_rm(&c->border_sectors, 0);

	printf("Growing civ %s into %s at %ldx%ld\n", c->name, s->name, s->x, s->y);

	return 0;
}

#define INITIAL_MIN_INTERCIV_DISTANCE_LY (10 * CIV_MIN_BORDER_WIDTH)
static void spawn_civilizations(struct universe *u, struct civ *civs)
{
	struct civ *c;
	struct sector *s;
	int success, tries;
	struct ptrlist neigh;
	struct list_head *lh;

	list_for_each_entry(c, &civs->list, list) {
		tries = 0;
		do {
			tries++;
			success = 1;
			s = ptrlist_random(&u->sectors);
			if (!s->owner) {
				ptrlist_init(&neigh);

				get_neighbouring_systems(&neigh, s, INITIAL_MIN_INTERCIV_DISTANCE_LY);
				struct sector *t;
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
		ptrlist_push(&c->sectors, s);
		ptrlist_push(&c->border_sectors, s);
		univ.inhabited_sectors++;
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

	goal_hab = ptrlist_len(&u->sectors) * UNIVERSE_CIV_FRAC;

	total_power = 0;
	list_for_each_entry(c, &civs->list, list) {
		total_power += c->power;
	}

	struct list_head growing_civs = LIST_HEAD_INIT(growing_civs);
	list_for_each_entry(c, &civs->list, list)
		list_add(&growing_civs, &c->growing);

	while (univ.inhabited_sectors < goal_hab && !list_empty(&growing_civs)) {
		list_for_each_entry_safe(c, _c, &growing_civs, growing) {
			if (mtrandom_ulong(total_power) >= c->power)
				continue;

			if (!grow_civ(u, c))
				univ.inhabited_sectors++;
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
		mprintf("  %s has %lu sectors (%.2f%%) with power %u\n", c->name, ptrlist_len(&c->sectors), ptrlist_len(&c->sectors)/(float)univ.inhabited_sectors*100, c->power);
	mprintf("%lu sectors of %lu are inhabited (%.2f%%)\n", univ.inhabited_sectors, ptrlist_len(&u->sectors), univ.inhabited_sectors/(float)ptrlist_len(&u->sectors)*100);
}

void civ_init(struct civ *c)
{
	memset(c, 0, sizeof(*c));
	ptrlist_init(&c->presectors);
	ptrlist_init(&c->availnames);
	ptrlist_init(&c->sectors);
	ptrlist_init(&c->border_sectors);
	INIT_LIST_HEAD(&c->list);
	INIT_LIST_HEAD(&c->growing);
}

void loadciv(struct civ *c, struct config *ctree)
{
	char *st;
	civ_init(c);

	ctree = ctree->sub;
	while (ctree) {
		if (strcmp(ctree->key, "NAME") == 0) {
			c->name = strdup(ctree->data);
		} else if (strcmp(ctree->key, "HOME") == 0) {
		} else if (strcmp(ctree->key, "POWER") == 0) {
			sscanf(ctree->data, "%d", &c->power);
		} else if (strcmp(ctree->key, "SECTOR") == 0) {
			/*	FIXME: We won't do this now as we don't support it
			        s = sector_load(ctree->sub);
			        ptrlist_push(c->presectors, s); */
		} else if (strcmp(ctree->key, "SNAME") == 0) {
			st = strdup(ctree->data);
			ptrlist_push(&c->availnames, st);
		}
		ctree = ctree->next;
	}
}

int civ_load_all(struct civ *civs)
{
	DIR *dirp;
	struct dirent *de;
	struct config *ctree;
	struct civ *cv;
	char* path = NULL;
	int pathlen = 0;
	int r = 0;

	if (!(dirp = opendir("civs")))
		die("%s", "opendir() on civs failed");

	while ((de = readdir(dirp)) != NULL) {
		if (de->d_name[0] != '.') {
			if ((int)strlen(de->d_name) > pathlen-6) {
				if (path != NULL)
					free(path);

				pathlen = strlen(de->d_name)+6;
				path = malloc(pathlen);
			}

			sprintf(path, "%s/%s", "civs", de->d_name);
			ctree = parseconfig(path);

			cv = malloc(sizeof(*cv));
			if (!cv) {
				r = -1;
				goto close;
			}

			loadciv(cv, ctree);
			destroyctree(ctree);
			list_add_tail(&(cv->list), &(civs->list));
		}
	}

close:
	if (path != NULL)
		free(path);
	closedir(dirp);

	return r;
}

void civ_free(struct civ *civ)
{
	char *c;
	struct list_head *lh;
	ptrlist_free(&civ->sectors);
	ptrlist_free(&civ->border_sectors);
	ptrlist_free(&civ->presectors);
	ptrlist_for_each_entry(c, &civ->availnames, lh)
		free(c);
	ptrlist_free(&civ->availnames);
	free(civ->name);
}
