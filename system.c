#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "log.h"
#include "universe.h"
#include "civ.h"
#include "system.h"
#include "planet.h"
#include "base.h"
#include "ptrlist.h"
#include "parseconfig.h"
#include "star.h"

void system_init(struct system *s)
{
	memset(s, 0, sizeof(*s));
	s->phi = 0.0;

	rb_init_node(&s->x_rbtree);
	ptrlist_init(&s->stars);
	ptrlist_init(&s->planets);
	ptrlist_init(&s->bases);
	ptrlist_init(&s->links);

	INIT_LIST_HEAD(&s->list);
}

void system_free(struct system *s) {
	struct list_head *lh;
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

struct system* system_load(const struct list_head * const config_root)
{
	struct config *conf;
	struct system *s;
	struct list_head *lh;
	struct planet *p;
	struct base *b;
	struct star *sol;
	int haspos = 0;

	s = malloc(sizeof(*s));
	if (!s)
		return NULL;
	system_init(s);

	list_for_each_entry(conf, config_root, list) {
		if (strcmp(conf->key, "SYSTEM") == 0) {

			s->name = NULL;

		} else if (strcmp(conf->key, "PLANET") == 0) {

			p = malloc(sizeof(*p));
			if (!p)
				goto err;

			planet_load(p, &conf->children);
			ptrlist_push(&s->planets, p);

		} else if (strcmp(conf->key, "BASE") == 0) {

			b = malloc(sizeof(*b));
			if (!b)
				goto err;

			loadbase(b, &conf->children);
			ptrlist_push(&s->bases, b);

		} else if (strcmp(conf->key, "STAR") == 0) {

			sol = malloc(sizeof(*sol));
			if (!sol)
				goto err;

			star_load(sol, &conf->children);
			ptrlist_push(&s->stars, sol);

		} else if (strcmp(conf->key, "POS") == 0) {

			struct config *child;
			list_for_each_entry(child, &conf->children, list) {
				if (strcmp(child->key, "x") == 0)
					s->x = child->l;
				else if (strcmp(child->key, "y") == 0)
					s->y = child->l;
			}
			haspos = 1;

		}
	}

	unsigned int totlum = 0;
	ptrlist_for_each_entry(sol, &s->stars, lh) {
		s->hab += sol->hab;
		totlum += sol->lumval;
	}
	if (!s->gname)
		die("%s", "required attribute missing in predefined system: gname");
	if (!haspos)
		die("required attribute missing in predefined system %s: position", s->name);

	s->hablow = star_gethablow(totlum);
	s->habhigh = star_gethabhigh(totlum);
	return s;

err:
	system_free(s);
	return NULL;
}

#define STELLAR_MUL_HAB -50
int system_create(struct system *s, char *name)
{
	struct star *sol;
	struct list_head *lh;

	system_init(s);

	s->name = strdup(name);

	if (star_populate_system(s)) {
		free(s->name);
		return -1;
	}

	s->hab = 0;

	unsigned int totlum = 0;
	ptrlist_for_each_entry(sol, &s->stars, lh) {
		s->hab += sol->hab;
		totlum += sol->lumval;
	}
	s->hablow = star_gethablow(totlum);
	s->habhigh = star_gethabhigh(totlum);

	if (planet_populate_system(s))
		return -1;

	printf("  Number of planets: %lu\n", ptrlist_len(&s->planets));

	return 0;
}

unsigned long system_distance(const struct system * const a, const struct system * const b) {
	long result = sqrt( (double)(b->x - a->x)*(b->x - a->x) +
			(double)(b->y - a->y)*(b->y - a->y) );

	if (result < 0)
		return -result;

	return result;
}
