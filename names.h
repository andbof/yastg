#ifndef _HAS_NAMES_H
#define _HAS_NAMES_H

#include "ptrarray.h"

struct name_list {
	struct ptrarray *prefix;
	struct ptrarray *first;
	struct ptrarray *second;
	struct ptrarray *suffix;
};

void names_init(struct name_list *l);
void names_free(struct name_list *l);
void names_load(struct name_list *l, char *prefix, char *first, char *second, char *suffix);
char* create_unique_name(struct name_list *l);

#endif
