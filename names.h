#ifndef _HAS_NAMES_H
#define _HAS_NAMES_H

#include "list.h"
#include "ptrarray.h"

struct name_list {
	struct ptrarray *prefix;
	struct ptrarray *first;
	struct ptrarray *second;
	struct ptrarray *suffix;
	struct list_head taken;
};

void names_init(struct name_list *l);
void names_free(struct name_list *l);
void names_load(struct name_list *l, const char * const prefix, const char * const first,
		const char * const second, const char * const suffix);
char* create_unique_name(struct name_list *l);
int is_names_loaded(struct name_list *l);

#endif
