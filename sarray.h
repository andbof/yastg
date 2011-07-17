#ifndef HAS_SARRAY_H
#define HAS_SARRAY_H

#define SARRAY_ALLOW_MULTIPLE 0
#define SARRAY_ENFORCE_UNIQUE 1

struct sarray {
  size_t elements;
  size_t allocated;
  size_t element_size;
  int maxkey;
  void (*freefnc)(void*);
  int (*sortfnc)(void*, void*);
  void* array;
};

struct sarray* sarray_init(size_t asize, int maxkey, void (*freefnc)(void*), int (*sortfnc)(void*, void*));

void sarray_move(struct sarray *a, struct sarray *b);

void* sarray_getbyid(struct sarray *a, void *key);
void* sarray_recgetbyid(struct sarray *a, void *key, size_t l, size_t u);
void* sarray_bubblegetbyid(struct sarray *a, void *key);

void* sarray_getprevbyid(struct sarray *a, void *key);
void* sarray_recgetprev(struct sarray *a, void *key, size_t l, size_t u);
void* sarray_bubblegetprev(struct sarray *a, void *key);

//void* sarray_getbyname(struct sarray *a, char *name);

void* sarray_getbypos(struct sarray *a, size_t pos);

struct ptr_num* sarray_getlnbyid(struct sarray *a, void *key);

int sarray_add(struct sarray *a, void *e);

void sarray_rmbyid(struct sarray *a, void *key);
void sarray_freebyid(struct sarray *a, void *key);
void sarray_rmbypos(struct sarray *a, size_t pos);
void sarray_rmbyptr(struct sarray *a, void* ptr);

void sarray_print(struct sarray *a);

void sarray_free(struct sarray *a);

#ifdef TEST
int sarray_test();
#endif

#endif
