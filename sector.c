#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "log.h"
#include "universe.h"
#include "civ.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "ptrlist.h"
#include "sarray.h"
#include "parseconfig.h"
#include "star.h"

struct sector* sector_init()
{
	struct sector *s;
	MALLOC_DIE(s, sizeof(*s));
	memset(s, 0, sizeof(*s));
	s->phi = 0.0;

	ptrlist_init(&s->stars);
	ptrlist_init(&s->planets);
	ptrlist_init(&s->bases);
	ptrlist_init(&s->links);

	INIT_LIST_HEAD(&s->list);
	return s;
}

void sector_free(void *ptr) {
	size_t st;
	struct list_head *lh;
	struct sector *s = ptr;
	struct star *sol;
	struct planet *planet;
	struct base *base;

	if (s->name)
		free(s->name);

	if (s->gname)
		free(s->gname);

	ptrlist_for_each_entry(sol, &s->stars, lh)
		star_free(sol);
	ptrlist_free(&s->stars);

	ptrlist_for_each_entry(planet, &s->planets, lh)
		planet_free(planet);
	ptrlist_free(&s->planets);

	ptrlist_for_each_entry(base, &s->bases, lh)
		base_free(base);
	ptrlist_free(&s->bases);

	ptrlist_free(&s->links);
	free(s);
}

struct sector* sector_load(struct config *ctree)
{
	struct sector *s = sector_init();
	struct list_head *lh;
	struct planet *p;
	struct base *b;
	struct star *sol;
	size_t i;
	int haspos = 0;
	while (ctree) {
		if (strcmp(ctree->key, "SECTOR") == 0) {
			s->name = NULL;
		} else if (strcmp(ctree->key, "PLANET") == 0) {
			p = loadplanet(ctree->sub);
			ptrlist_push(&s->planets, p);
		} else if (strcmp(ctree->key, "BASE") == 0) {
			b = loadbase(ctree->sub);
			ptrlist_push(&s->bases, b);
		} else if (strcmp(ctree->key, "STAR") == 0) {
			sol = loadstar(ctree->sub);
			ptrlist_push(&s->stars, sol);
		} else if (strcmp(ctree->key, "POS") == 0) {
			sscanf(ctree->data, "%lu %lu", &s->x, &s->y);
			haspos = 1;
		}
		ctree = ctree->next;
	}

	unsigned int totlum = 0;
	ptrlist_for_each_entry(sol, &s->stars, lh) {
		s->hab += sol->hab;
		totlum += sol->lumval;
	}
	if (!s->gname)
		die("%s", "required attribute missing in predefined sector: gname");
	if (!haspos)
		die("required attribute missing in predefined sector %s: position", s->name);

	s->hablow = star_gethablow(totlum);
	s->habhigh = star_gethabhigh(totlum);
	return s;
}

#define STELLAR_MUL_HAB -50
struct sector* sector_create(char *name)
{
	struct star *sol;
	struct sector *s = sector_init();
	struct list_head *lh;
	s->name = strdup(name);
	star_populate_sector(s);
	s->hab = 0;

	unsigned int totlum = 0;
	ptrlist_for_each_entry(sol, &s->stars, lh) {
		s->hab += sol->hab;
		totlum += sol->lumval;
	}
	s->hablow = star_gethablow(totlum);
	s->habhigh = star_gethabhigh(totlum);

	planet_populate_sector(s);
	printf("  Number of planets: %lu\n", ptrlist_len(&s->planets));

	return s;
}

void sector_move(struct sector *s, long x, long y)
{
	struct ulong_ptr uptr;
	struct double_ptr dptr;
	struct sector *stmp;
	size_t st;
	s->x = x;
	s->y = y;
	uptr.ptr = s;
	uptr.i = XYTORAD(s->x, s->y);
	dptr.ptr = s;
	dptr.i = XYTOPHI(s->x, s->y);
	/* We need to make sure we're not adding this sector twice to srad and sphi
	   FIXME: Scales really badly */
	for (st = 0; st < univ.srad->elements; st++) {
		stmp = sarray_getbypos(univ.srad, st);
		if (stmp == s) {
			sarray_rmbypos(univ.srad, st);
			break;
		}
	}
	for (st = 0; st < univ.sphi->elements; st++) {
		stmp = sarray_getbypos(univ.sphi, st);
		if (stmp == s) {
			sarray_rmbypos(univ.sphi, st);
			break;
		}
	}
	/* Now update the coordinates and add them to srad and sphi */
	s->r = uptr.i;
	sarray_add(univ.srad, &uptr);
	s->phi = dptr.i;
	sarray_add(univ.sphi, &dptr);
}

unsigned long sector_distance(struct sector *a, struct sector *b) {
	long result = sqrt( (double)(b->x - a->x)*(b->x - a->x) + (double)(b->y - a->y)*(b->y - a->y) );
	if (result < 0)
		return -result;
	return result;
}
