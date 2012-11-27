#include <stdlib.h>
#include <string.h>
#include "base.h"
#include "common.h"
#include "data.h"
#include "htable.h"
#include "log.h"
#include "parseconfig.h"
#include "planet.h"
#include "sarray.h"
#include "universe.h"

struct base* loadbase(struct configtree *ctree)
{
	struct base *b;
	MALLOC_DIE(b, sizeof(*b));
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
	htable_rm(univ->basenames, b->name);
	free(b->name);
	ptrlist_free(&b->inventory);
	ptrlist_free(&b->players);
	free(b);
}

static struct base* base_create()
{
	struct base *base;
	MALLOC_DIE(base, sizeof(*base));
	memset(base, 0, sizeof(*base));
	ptrlist_init(&base->inventory);
	ptrlist_init(&base->players);
	return base;
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
		base->name = names_generate(&univ->avail_base_names);
	} while (htable_get(univ->basenames, base->name));
}

void base_populate_planet(struct planet* planet)
{
	struct base *b;
	int num = 10; /* FIXME */

	pthread_rwlock_wrlock(&univ->basenames->lock);

	for (int i = 0; i < num; i++) {
		b = base_create();
		base_genesis(b, planet);
		ptrlist_push(&planet->bases, b);
		htable_add(univ->basenames, b->name, b);
	}

	pthread_rwlock_unlock(&univ->basenames->lock);
}
