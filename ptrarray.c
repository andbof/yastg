#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "defines.h"
#include "log.h"
#include "ptrarray.h"
#include "sector.h"

struct ptrarray* ptrarray_init(size_t asize)
{
	struct ptrarray *a;
	MALLOC_DIE(a, sizeof(*a));
	a->elements = 0;
	a->allocated = asize;

	if (asize > 0) {
		MALLOC_DIE(a->array, asize * sizeof(void*));
	} else {
		a->array = NULL;
	}

	return a;
}

#define PTRARRAY_REALLOC_STEP 1
void ptrarray_incsize(struct ptrarray *a)
{
	if (a->array) {
		REALLOC_DIE(a->array, (a->allocated + PTRARRAY_REALLOC_STEP) * sizeof(void*));
	} else {
		MALLOC_DIE(a->array, PTRARRAY_REALLOC_STEP * sizeof(void*));
	}
	a->allocated += PTRARRAY_REALLOC_STEP;
}

void ptrarray_push(struct ptrarray *a, void *e)
{
	void *ptr;
	if (a->elements == a->allocated)
		ptrarray_incsize(a);
	MEMCPY_DIE(a->array + a->elements * sizeof(void*), &e, sizeof(void*));
	a->elements++;
}

void* ptrarray_pop(struct ptrarray *a)
{
	void **ptr;
	if (a->elements > 0) {
		ptr = a->array + (a->elements - 1) * sizeof(void*);
		a->elements--;
		return *ptr;
	} else {
		return NULL;
	}
}

void* ptrarray_get(struct ptrarray *a, size_t n)
{
	if (a->array == NULL)
		assert(a->elements == 0);
	assert(n <= a->elements);
	printf("ptrarray_get(): a = %p, a->array = %p, a->elements = %zu\n", a, a->array, a->elements);
	return *(void**)(a->array + (sizeof(void*) * n));
}

void ptrarray_rm(struct ptrarray *a, size_t n)
{
	if (n < a->elements)
		MEMMOVE_DIE(a->array + n * sizeof(void*), a->array + (n + 1) * sizeof(void*), (a->elements - 1) * sizeof(void*) - n * sizeof(void*));
	a->elements--;
}

void ptrarray_free(struct ptrarray *a)
{
	assert(a != NULL);
	/* It is possible for a->array to be NULL if ptrarray_init was called with
	 * asize = 0 and the array hasn't been used. */
	if (a->array)
		free(a->array);
	free(a);
}
