#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "log.h"
#include "universe.h"
#include "civ.h"
#include "system.h"
#include "planet.h"
#include "port.h"
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
	ptrlist_init(&s->ports);
	ptrlist_init(&s->links);

	INIT_LIST_HEAD(&s->list);
}

void system_free(struct system *s) {
	struct list_head *lh;
	struct star *sol;
	struct planet *planet;
	struct port *port;

	free(s->name);
	free(s->gname);

	ptrlist_for_each_entry(sol, &s->stars, lh)
		star_free(sol);
	ptrlist_free(&s->stars);

	ptrlist_for_each_entry(planet, &s->planets, lh)
		planet_free(planet);
	ptrlist_free(&s->planets);

	ptrlist_for_each_entry(port, &s->ports, lh)
		port_free(port);
	ptrlist_free(&s->ports);

	ptrlist_free(&s->links);
	free(s);
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
