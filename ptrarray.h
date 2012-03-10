#ifndef HAS_PTRARRAY_H
#define HAS_PTRARRAY_H

struct ptrarray {
	size_t elements;
	size_t allocated;
	void* array;
};

struct ptrarray* ptrarray_init(size_t asize);

void ptrarray_push(struct ptrarray *a, void *e);
void* ptrarray_pop(struct ptrarray *a);
void* ptrarray_get(struct ptrarray *a, size_t n);

void ptrarray_rm(struct ptrarray *a, size_t n);

void ptrarray_free(struct ptrarray *a);

#endif
