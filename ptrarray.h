#ifndef _HAS_PTRARRAY_H
#define _HAS_PTRARRAY_H

#include <stdlib.h>

struct ptrarray {
	size_t alloc;
	size_t used;
	void* array[];
};

struct ptrarray* ptrarray_create();
void ptrarray_free(struct ptrarray *a);
struct ptrarray* ptrarray_add(struct ptrarray *a, void *ptr);
struct ptrarray* file_to_ptrarray(const char * const fn, struct ptrarray *a);

static inline void* ptrarray_get(struct ptrarray *a, unsigned int idx) {
	if (idx >= a->used)
		return NULL;

	return a->array[idx];
}

#endif
