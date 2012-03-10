#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "defines.h"
#include "log.h"
#include "universe.h"
#include "civ.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "ptrarray.h"
#include "sarray.h"
#include "parseconfig.h"
#include "id.h"
#include "star.h"

struct sector* sector_init() {
	struct sector *s;
	MALLOC_DIE(s, sizeof(*s));
	memset(s, 0, sizeof(*s));
	s->phi = 0.0;
	s->links = ptrarray_init(0);
	return s;
}

void sector_free(void *ptr) {
	size_t st;
	struct sector *s = ptr;
	if (s->name)
		free(s->name);
	if (s->gname)
		free(s->gname);
	for (st = 0; st < s->stars->elements; st++)
		star_free(ptrarray_get(s->stars, st));
	ptrarray_free(s->stars);
	for (st = 0; st < s->planets->elements; st++)
		planet_free(ptrarray_get(s->planets, st));
	ptrarray_free(s->planets);
	for (st = 0; st < s->bases->elements; st++)
		base_free(ptrarray_get(s->bases, st));
	ptrarray_free(s->bases);
	ptrarray_free(s->links);
	free(s);
}

struct sector* sector_load(struct configtree *ctree) {
	struct sector *s = sector_init();
	struct planet *p;
	struct base *b;
	struct star *sol;
	size_t i;
	int haspos = 0;
	s->stars = ptrarray_init(0);
	s->planets = ptrarray_init(0);
	s->bases = ptrarray_init(0);
	while (ctree) {
		if (strcmp(ctree->key, "GNAME") == 0) {
			s->gname = strdup(ctree->data);
			s->name = NULL;
		} else if (strcmp(ctree->key, "PLANET") == 0) {
			p = loadplanet(ctree->sub);
			ptrarray_push(s->planets, p);
		} else if (strcmp(ctree->key, "BASE") == 0) {
			b = loadbase(ctree->sub);
			ptrarray_push(s->bases, b);
		} else if (strcmp(ctree->key, "STAR") == 0) {
			sol = loadstar(ctree->sub);
			ptrarray_push(s->stars, sol);
		} else if (strcmp(ctree->key, "POS") == 0) {
			sscanf(ctree->data, "%lu %lu", &s->x, &s->y);
			haspos = 1;
		}
		ctree = ctree->next;
	}
	for (i = 0; i < s->stars->elements; i++)
		s->hab += ((struct star*)ptrarray_get(s->stars, i))->hab;
	if (!s->gname)
		die("%s", "required attribute missing in predefined sector: gname");
	if (!haspos)
		die("required attribute missing in predefined sector %s: position", s->name);
	s->hablow = ((struct star*)ptrarray_get(s->stars, 0))->hablow;
	s->habhigh = ((struct star*)ptrarray_get(s->stars, 0))->habhigh;
	s->snowline = ((struct star*)ptrarray_get(s->stars, 0))->snowline;
	return s;
}

struct sector* sector_create(char *name) {
	int i;
	struct star *sol;
	struct sector *s = sector_init();
	s->name = strdup(name);
	s->stars = createstars();
	s->hab = 0;
	for (i = 0; i < s->stars->elements; i++) {
		sol = ptrarray_get(s->stars, i);
		s->hab += sol->hab;
		MALLOC_DIE(sol->name, strlen(s->name)+3);
		sprintf(sol->name, "%s %c", s->name, i+65);
	}
	s->hab -= STELLAR_MUL_HAB*(i-1);
	s->hablow = ((struct star*)ptrarray_get(s->stars, 0))->hablow;
	s->habhigh = ((struct star*)ptrarray_get(s->stars, 0))->habhigh;
	s->snowline = ((struct star*)ptrarray_get(s->stars, 0))->snowline;
	s->planets = createplanets(s);
	s->bases = ptrarray_init(0);
	// FIXME: bases
	return s;
}

void sector_move(struct sector *s, long x, long y) {
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
	// We need to make sure we're not adding this sector twice to srad and sphi
	// FIXME: Scales really badly
	for (st = 0; st < univ->srad->elements; st++) {
		stmp = sarray_getbypos(univ->srad, st);
		if (stmp == s) {
			sarray_rmbypos(univ->srad, st);
			break;
		}
	}
	for (st = 0; st < univ->sphi->elements; st++) {
		stmp = sarray_getbypos(univ->sphi, st);
		if (stmp == s) {
			sarray_rmbypos(univ->sphi, st);
			break;
		}
	}
	// Now update the coordinates and add them to srad and sphi
	s->r = uptr.i;
	sarray_add(univ->srad, &uptr);
	s->phi = dptr.i;
	sarray_add(univ->sphi, &dptr);
}

unsigned long sector_distance(struct sector *a, struct sector *b) {
	long result = sqrt( (double)(b->x - a->x)*(b->x - a->x) + (double)(b->y - a->y)*(b->y - a->y) );
	if (result < 0)
		return -result;
	return result;
}
