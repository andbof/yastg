#ifndef _HAS_SARRAY_H
#define _HAS_SARRAY_H

#define SARRAY_ALLOW_MULTIPLE 0
#define SARRAY_ENFORCE_UNIQUE 1

struct sarray {
	unsigned long elements;
	unsigned long allocated;
	unsigned long element_size;
	int maxkey;
	void (*freefnc)(void*);
	int (*sortfnc)(void*, void*);
	void* array;
};

struct sarray* sarray_init(unsigned long esize, unsigned long asize, int maxkey, void (*freefnc)(void*), int (*sortfnc)(void*, void*));

void* sarray_getbypos(struct sarray *a, unsigned long pos);

struct ptr_num* sarray_getlnbyid(struct sarray *a, void *key);

int sarray_add(struct sarray *a, void *e);

void sarray_rmbypos(struct sarray *a, unsigned long pos);

void sarray_free(struct sarray *a);

int sort_ulong(void *a, void *b);
int sort_double(void *a, void *b);

#endif
