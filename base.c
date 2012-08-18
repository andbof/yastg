#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "log.h"
#include "base.h"
#include "sarray.h"
#include "parseconfig.h"

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
