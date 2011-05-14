#ifndef HAS_ARRAY_H
#define HAS_ARRAY_H

struct array {
  size_t elements;
  size_t allocated;
  size_t element_size;
  void (*freefnc)(void*);
  void* array;
};

struct array* array_init(size_t esize, size_t asize, void (*freefnc)(void*));

void array_push(struct array *a, void *e);

void array_rm(struct array *a, size_t key);

void* array_get(struct array *a, size_t n);

void array_destroy(struct array *a);

void array_free(struct array *a);

#ifdef TEST
int array_test();
#endif

#endif
