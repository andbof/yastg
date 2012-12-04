#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "base.h"
#include "common.h"
#include "data.h"
#include "stringtree.h"
#include "log.h"
#include "parseconfig.h"
#include "planet.h"
#include "universe.h"

struct base* loadbase(struct base *b, struct config *ctree)
{
	memset(b, 0, sizeof(*b));
	ptrlist_init(&b->inventory);
	ptrlist_init(&b->players);

	b->docks = 0;
	while (ctree) {
		if (strcmp(ctree->key, "NAME") == 0)
			b->name = strdup(ctree->data);
		else if (strcmp(ctree->key, "TYPE") == 0)
			b->type = ctree->data[0];
		else if (strcmp(ctree->key, "DOCKS") == 0)
			sscanf(ctree->data, "%d", &(b->docks));
		/* FIXME: inventory + players */
		ctree = ctree->next;
	}
	return b;
}

void base_free(struct base *b)
{
	st_rm_string(&univ.basenames, b->name);
	free(b->name);
	ptrlist_free(&b->inventory);
	ptrlist_free(&b->players);
	free(b);
}

static void base_init(struct base *base)
{
	memset(base, 0, sizeof(*base));
	ptrlist_init(&base->inventory);
	ptrlist_init(&base->players);
}

static void base_genesis(struct base *base, struct planet *planet)
{
	base->planet = planet;
	base->type = 0; /* FIXME */

	struct base_type *type = &base_types[base->type];
	base->docks = 1; /* FIXME */

	/* FIXME: limit loop */
	do {
		if (base->name)
			free(base->name);
		base->name = create_unique_name(&univ.avail_base_names);
	} while (st_lookup_exact(&univ.basenames, base->name));
}

void base_populate_planet(struct planet* planet)
{
	struct base *b;
	int num = 10; /* FIXME */

	pthread_rwlock_wrlock(&univ.basenames_lock);

	for (int i = 0; i < num; i++) {
		b = malloc(sizeof(*b));
		if (!b)
			goto unlock;
		base_init(b);
		base_genesis(b, planet);
		ptrlist_push(&planet->bases, b);
		st_add_string(&univ.basenames, b->name, b);
	}

unlock:
	pthread_rwlock_unlock(&univ.basenames_lock);
}
