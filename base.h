#ifndef _HAS_BASE_H
#define _HAS_BASE_H

#include "ptrlist.h"
#include "parseconfig.h"
#include "base_type.h"

struct base {
	char *name;
	struct base_type *type;
	int docks;
	struct planet *planet;
	struct system *system;
	struct ptrlist inventory;
	struct ptrlist players;
};

void base_populate_planet(struct planet* planet);

void base_free(struct base *b);

#endif
