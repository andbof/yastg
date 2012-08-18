#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "log.h"
#include "base.h"
#include "sarray.h"
#include "parseconfig.h"
#include "planet.h"

struct base* loadbase(struct configtree *ctree)
{
	struct base *b;
	MALLOC_DIE(b, sizeof(*b));
	memset(b, 0, sizeof(*b));
	b->inventory = NULL;	/* FIXME */
	b->players = NULL;	/* FIXME */
	b->docks = 0;		/* FIXME */
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

void base_free(void *ptr)
{
	struct base *b = ptr;
	free(b->name);
	if (b->inventory) free(b->inventory);
	if (b->players) free(b->players);
	free(b);
}

static struct base* base_create()
{
	struct base *base;
	MALLOC_DIE(base, sizeof(*base));
	memset(base, 0, sizeof(*base));
	return base;
}

static void base_init(struct base *base)
{
}

void base_populate_planet(struct planet* planet)
{
	struct base *base;
}
