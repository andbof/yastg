#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "log.h"
#include "ptrarray.h"
#include "sector.h"

#define PTRARRAY_REALLOC_STEP 1

struct ptrarray* ptrarray_init(size_t asize) {
  struct ptrarray *a;
  MALLOC_DIE(a, sizeof(*a));
  a->elements = 0;
  a->allocated = asize;
  MALLOC_DIE(a->array, asize * sizeof(void*));
  return a;
}

void ptrarray_incsize(struct ptrarray *a) {
  unsigned int step = PTRARRAY_REALLOC_STEP;
  REALLOC_DIE(a->array, (a->allocated + step) * sizeof(void*));
  a->allocated += step;
}

void ptrarray_push(struct ptrarray *a, void *e) {
  void *ptr;
  if (a->elements == a->allocated) ptrarray_incsize(a);
  memcpy(a->array + a->elements * sizeof(void*), &e, sizeof(void*));
  a->elements++;
}

void ptrarray_rm(struct ptrarray *a, size_t n) {
  if (n < a->elements) {
    memmove(a->array + n * sizeof(void*), a->array + (n + 1) * sizeof(void*), (a->elements - 1) * sizeof(void*) - n * sizeof(void*));
  }
  a->elements--;
}

void* ptrarray_get(struct ptrarray *a, size_t n) {
  return *(void**)(a->array + (sizeof(void*) * n));
}

void ptrarray_free(struct ptrarray *a) {
  free(a->array);
  free(a);
}

