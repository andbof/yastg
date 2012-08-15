#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "defines.h"
#include "log.h"
#include "mtrandom.h"
#include "civ.h"
#include "ptrarray.h"
#include "sector.h"
#include "sarray.h"
#include "array.h"
#include "parseconfig.h"
#include "id.h"
#include "universe.h"

#define CIV_GROW_MIN 10
#define CIV_GROW_STEP 10
static void growciv(struct universe *u, struct civ *c)
{
	struct sector *s, *t;
	struct ptrarray *neigh;
	size_t i;
	unsigned long rad = CIV_GROW_MIN;
	t = ptrarray_get(c->sectors, mtrandom_sizet(c->sectors->elements));
	do {
		s = NULL;
		neigh = getneighbours(t, rad);
		for (i = 0; i < neigh->elements; i++) {
			s = ptrarray_get(neigh, i);
			if (!s->owner)
				break;
		}
		if ((s == NULL) || (s->owner))
			rad += CIV_GROW_STEP;
		ptrarray_free(neigh);
	} while ((s == NULL) || (s->owner));
	s->owner = c;
	linksectors(s, t);
	ptrarray_push(c->sectors, s);
}
#define UNIVERSE_CIV_FRAC 0.4
#define UNIVERSE_MIN_INTERCIV_DISTANCE 100
void civ_spawncivs(struct universe *u, struct array *civs)
{
	size_t i, j, k, nhab, chab, tpow;
	struct sector *s;
	struct ptrarray *neigh;
	struct civ *c;
	nhab = u->sectors->elements * UNIVERSE_CIV_FRAC;
	/* Calculate total civilization power (we need this to be able
	   to get their relative values) */
	tpow = 0;
	for (i = 0; i < civs->elements; i++) {
		tpow += ((struct civ*)array_get(civs, i))->power;
	}

	/* First, spawn all civilizations in separate home systems */
	for (i = 0; i < civs->elements; i++) {
		c = array_get(civs, i);
		do {
			k = 1;
			s = ptrarray_get(u->sectors, mtrandom_sizet(u->sectors->elements));
			if (!s->owner) {
				neigh = getneighbours(s, UNIVERSE_MIN_INTERCIV_DISTANCE);
				for (j = 0; j < neigh->elements; j++) {
					if (((struct sector*)ptrarray_get(neigh, j))->owner != 0) {
						k = 0;
						break;
					}
				}
				ptrarray_free(neigh);
			} else {
				k = 0;
			}
		} while (!k);
		printf("Chose %s as home system for %s\n", s->name, c->name);
		s->owner = c;
		c->home = s;
		ptrarray_push(c->sectors, s);
	}

	mprintf("Growing civilizations ...\n");
	chab = civs->elements;
	while (chab < nhab) {
		for (i = 0; i < civs->elements; i++) {
			c = array_get(civs, i);
			if (mtrandom_sizet(tpow) < c->power) {
				growciv(u, c);
				chab++;
			}
		}
	}
	mprintf("done.\n");

	printf("Civilization stats:\n");
	for (i = 0; i < civs->elements; i++) {
		c = array_get(civs, i);
		printf("  %s has %zu sectors (%.2f%%) with power %u\n", c->name, c->sectors->elements, (float)c->sectors->elements/chab*100, c->power);
	}
	printf("%zu sectors of %zu are inhabited (%.2f%%)\n", chab, u->sectors->elements, (float)chab/u->sectors->elements*100);
}

struct civ* loadciv(struct configtree *ctree)
{
	struct civ *c;
	MALLOC_DIE(c, sizeof(*c));
	struct sector *s;
	char *st;
	c->presectors = ptrarray_init(0);
	c->availnames = ptrarray_init(0);
	c->sectors = ptrarray_init(0);
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
			        ptrarray_push(c->presectors, s); */
		} else if (strcmp(ctree->key, "SNAME") == 0) {
			st = strdup(ctree->data);
			ptrarray_push(c->availnames, st);
		}
		ctree = ctree->next;
	}
	return c;
}

struct array* loadcivs()
{
	DIR *dirp;
	struct dirent *de;
	struct configtree *ctree;
	struct array *a = array_init(sizeof(struct civ), 0, &civ_free);
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
			array_push(a, cv);
			free(cv);		/* All data, including pointers to others objects, have been copied. We need to free cv now. */
		}
	}
	if (path != NULL) free(path);
	closedir(dirp);
	return a;
}

void civ_free(void *ptr)
{
	struct civ* c = ptr;
	size_t st;
	ptrarray_free(c->sectors);
	ptrarray_free(c->presectors);
	for (st = 0; st < c->availnames->elements; st++)
		free(ptrarray_get(c->availnames, st));
	ptrarray_free(c->availnames);
	free(c->name);
}
