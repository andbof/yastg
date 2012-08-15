#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <assert.h>
#include "defines.h"
#include "log.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "ptrarray.h"
#include "parseconfig.h"
#include "mtrandom.h"

struct planet* initplanet()
{
	struct planet *p;
	MALLOC_DIE(p, sizeof(*p));
	memset(p, 0, sizeof(*p));
	p->bases = ptrarray_init(0);
	p->moons = ptrarray_init(0);
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
			ptrarray_push(p->bases, b);
		}
		ctree = ctree->next;
	}
	return p;
}

void planet_free(void *ptr)
{
	assert(ptr != NULL);
	struct planet *p = ptr;
	ptrarray_free(p->bases);
	ptrarray_free(p->moons);
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

struct ptrarray* createplanets(struct sector* s)
{
	int num = 0;
	struct planet *p;
	struct ptrarray* planets = ptrarray_init(0);
	while ((mtrandom_sizet(SIZE_MAX) < SIZE_MAX/PLANET_ODDS) && (num < PLANET_NUM_MAX)) {
		p = createplanet(s);
		ptrarray_push(planets, p);
	}
	return planets;
}
