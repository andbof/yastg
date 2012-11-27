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
#include "sarray.h"
#include "parseconfig.h"
#include "universe.h"
#include "list.h"

#define CIV_GROW_MIN 10
#define CIV_GROW_STEP 10
static void growciv(struct universe *u, struct civ *c)
{
	struct sector *s, *t;
	struct ptrlist *neigh;
	struct list_head *lh;
	size_t i;
	unsigned long rad = CIV_GROW_MIN;
	t = ptrlist_random(&c->sectors);
	do {
		s = NULL;
		neigh = getneighbours(t, rad);
		ptrlist_for_each_entry(s, neigh, lh) {
			if (!s->owner)
				break;
		}
		if ((s == NULL) || (s->owner))
			rad += CIV_GROW_STEP;
		ptrlist_free(neigh);
		free(neigh);
	} while ((s == NULL) || (s->owner));
	s->owner = c;
	linksectors(s, t);
	ptrlist_push(&c->sectors, s);
}

#define UNIVERSE_CIV_FRAC 0.4
#define UNIVERSE_MIN_INTERCIV_DISTANCE 100
void civ_spawncivs(struct universe *u, struct civ *civs)
{
	size_t i, j, k;
	unsigned long chab, nhab, tpow;
	struct sector *s;
	struct ptrlist *neigh;
	struct civ *c;
	nhab = ptrlist_len(&u->sectors) * UNIVERSE_CIV_FRAC;
	/* Calculate total civilization power (we need this to be able
	   to get their relative values) */
	tpow = 0;
	list_for_each_entry(c, &civs->list, list) {
		tpow += c->power;
	}

	/* First, spawn all civilizations in separate home systems */
	list_for_each_entry(c, &civs->list, list) {
		do {
			k = 1;
			s = ptrlist_random(&u->sectors);
			if (!s->owner) {
				neigh = getneighbours(s, UNIVERSE_MIN_INTERCIV_DISTANCE);
				struct sector *t;
				struct list_head *lh;
				ptrlist_for_each_entry(t, neigh, lh) {
					if (t->owner != 0) {
						k = 0;
						break;
					}
				}
				ptrlist_free(neigh);
				free(neigh);
			} else {
				k = 0;
			}
		} while (!k);
		printf("Chose %s as home system for %s\n", s->name, c->name);
		s->owner = c;
		c->home = s;
		ptrlist_push(&c->sectors, s);
	}

	mprintf("Growing civilizations ...\n");
	/* Each civ starts with one sector, current number of habitated sector is therefore == number of civs */
	chab = list_len(&civs->list);
	while (chab < nhab) {
		list_for_each_entry(c, &civs->list, list) {
			if (mtrandom_ulong(tpow) < c->power) {
				growciv(u, c);
				chab++;
			}
		}
	}
	mprintf("done.\n");

	struct list_head *lh;
	printf("Civilization stats:\n");
	list_for_each_entry(c, &civs->list, list)
		printf("  %s has %lu sectors (%.2f%%) with power %u\n", c->name, ptrlist_len(&c->sectors), (float)ptrlist_len(&c->sectors)/chab*100, c->power);
	printf("%lu sectors of %lu are inhabited (%.2f%%)\n", chab, ptrlist_len(&u->sectors), (float)chab/ptrlist_len(&u->sectors)*100);
}

struct civ* civ_create()
{
	struct civ *c;
	MALLOC_DIE(c, sizeof(*c));
	memset(c, 0, sizeof(*c));
	ptrlist_init(&c->presectors);
	ptrlist_init(&c->availnames);
	ptrlist_init(&c->sectors);
	INIT_LIST_HEAD(&c->list);
	return c;
}

struct civ* loadciv(struct configtree *ctree)
{
	struct civ *c = civ_create();
	struct sector *s;
	char *st;
	ctree=ctree->sub;
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
	return c;
}

int civ_load_all(struct civ *civs)
{
	DIR *dirp;
	struct dirent *de;
	struct configtree *ctree;
	struct civ *cv;
	char* path = NULL;
	int pathlen = 0;
	if (!(dirp = opendir("civs"))) die("%s", "opendir() on civs failed");
	while ((de = readdir(dirp)) != NULL) {
		if (de->d_name[0] != '.') {
			if ((int)strlen(de->d_name) > pathlen-6) {
				if (path != NULL) free(path);
				pathlen = strlen(de->d_name)+6;
				path = malloc(pathlen);
			}
			sprintf(path, "%s/%s", "civs", de->d_name);
			ctree = parseconfig(path);
			cv = loadciv(ctree);
			destroyctree(ctree);
			list_add_tail(&(cv->list), &(civs->list));
		}
	}
	if (path != NULL)
		free(path);
	closedir(dirp);

	return 0;
}

void civ_free(struct civ *civ)
{
	size_t st;
	char *c;
	struct list_head *lh;
	ptrlist_free(&civ->sectors);
	ptrlist_free(&civ->presectors);
	ptrlist_for_each_entry(c, &civ->availnames, lh)
		free(c);
	ptrlist_free(&civ->availnames);
	free(civ->name);
	free(civ);
}
