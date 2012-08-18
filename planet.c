#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "common.h"
#include "log.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "ptrlist.h"
#include "parseconfig.h"
#include "mtrandom.h"

struct planet* initplanet()
{
	struct planet *p;
	MALLOC_DIE(p, sizeof(*p));
	memset(p, 0, sizeof(*p));
	p->bases = ptrlist_init();
	p->moons = ptrlist_init();
	return p;
}

struct planet* loadplanet(struct configtree *ctree)
{
	struct base *b;
	struct planet *p = initplanet();
	while (ctree) {
		if (strcmp(ctree->key, "NAME") == 0) {
			p->name = strdup(ctree->data);
		} else if (strcmp(ctree->key, "TYPE") == 0) {
			p->type = ctree->data[0];
		} else if (strcmp(ctree->key, "BASE") == 0) {
			b = loadbase(ctree->sub);
			ptrlist_push(p->bases, b);
		}
		ctree = ctree->next;
	}
	return p;
}

void planet_free(struct planet *p)
{
	assert(p != NULL);
	ptrlist_free(p->bases);
	ptrlist_free(p->moons);
	free(p->name);
	free(p);
}

#define PLANET_ODDS 7
#define PLANET_NUM_MAX 11

struct planet* createplanet(struct sector* s)
{
	struct planet *p;
	MALLOC_DIE(p, sizeof(*p));
	return p;
}

struct ptrlist* createplanets(struct sector* s)
{
	int num = 0;
	struct planet *p;
	struct ptrlist* planets = ptrlist_init();
	while ((mtrandom_uint(UINT_MAX) < UINT_MAX/PLANET_ODDS) && (num < PLANET_NUM_MAX)) {
		p = createplanet(s);
		ptrlist_push(planets, p);
	}
	return planets;
}
