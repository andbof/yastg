#ifndef HAS_SARRAY_H
#define HAS_SARRAY_H

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

void sarray_move(struct sarray *a, struct sarray *b);

void* sarray_getbyid(struct sarray *a, void *key);
void* sarray_recgetbyid(struct sarray *a, void *key, unsigned long l, unsigned long u);
void* sarray_bubblegetbyid(struct sarray *a, void *key);

void* sarray_getprevbyid(struct sarray *a, void *key);
void* sarray_recgetprev(struct sarray *a, void *key, unsigned long l, unsigned long u);
void* sarray_bubblegetprev(struct sarray *a, void *key);

void* sarray_getbypos(struct sarray *a, unsigned long pos);

struct ptr_num* sarray_getlnbyid(struct sarray *a, void *key);

int sarray_add(struct sarray *a, void *e);

void sarray_rmbyid(struct sarray *a, void *key);
void sarray_freebyid(struct sarray *a, void *key);
void sarray_rmbypos(struct sarray *a, unsigned long pos);
void sarray_rmbyptr(struct sarray *a, void* ptr);

void sarray_print(struct sarray *a);

void sarray_free(struct sarray *a);

#ifdef TEST
int sarray_test();
#endif

#endif
