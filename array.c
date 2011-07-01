#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "log.h"
#include "array.h"
#include "sector.h"
#include "shuffle.h"

#define ARRAY_REALLOC_STEP 1

struct array* array_init(size_t esize, size_t asize, void (*freefnc)(void*)) {
  struct array* a = malloc(sizeof(*a)); // FIXME
  a->elements = 0;
  a->element_size = esize;
  a->allocated = asize;
  a->freefnc = freefnc;
  MALLOC_DIE(a->array, asize*esize);
  return a;
}

void array_incsize(struct array *a) {
  unsigned int step = ARRAY_REALLOC_STEP;
  REALLOC_DIE(a->array, (a->allocated+step)*a->element_size);
  a->allocated += step;
}

void array_push(struct array *a, void *e) {
  if (a->elements == a->allocated) array_incsize(a);
  memcpy(a->array+a->elements*a->element_size, e, a->element_size);
  a->elements++;
}

void array_rm(struct array *a, size_t key) {
  if (key < a->elements) {
    memmove(a->array+key*a->element_size, a->array+(key+1)*a->element_size, (a->elements-1)*a->element_size-key*a->element_size);
  }
  a->elements--;
}

void* array_get(struct array *a, size_t n) {
  return a->array+(a->element_size*n);
}

void array_free(struct array *a) {
  size_t l;
  if (a->freefnc) {
    for (l = 0; l < a->elements; l++) {
//      printf("Calling freefnc for element %zu of %zu @ %p; a @ %p; a->array @ %p\n", l, a->elements, a->array+l*a->element_size, a, a->array);
      (*(a->freefnc))(a->array+l*a->element_size);
    }
  }
  free(a->array);
}

#ifdef TEST
#define ARRAY_N 16
int array_test() {
  struct foo {
    size_t id;
    int gurka;
  } *u[ARRAY_N];
  void *ptr;
  int i, j, k, l;
  // *a will be our test array, allocate memory for it and all its elements.
  struct array *a;
  a = array_init(sizeof(struct foo), 0, &free);
  // u[] is our array with test elements to add and remove
  for (i = 0; i < ARRAY_N; i++) {
    MALLOC_DIE(u[i], sizeof(struct foo));
    u[i]->id = i;
  }
  // Randomize u[] to add elements in a random order
  shuffleptr((void**)u, ARRAY_N);
  // Add all elements in u[] to array a
  for (i = 0; i < ARRAY_N; i++) {
    array_push(a, u[i]);
    // For each insertion, make sure all elements in array are still there
    for (j = 0; j < i; j++) {
      if (!(ptr = array_get(a, j))) {
	bug("element %d not found after adding %d elements", j, i);
      }
    }
  }
  // Randomize u[] again before deleting elements
  shuffleptr((void**)u, sizeof(struct foo));
  // Now we delete elements from a in the order they are found in u[].
  // For each deletion, we run through the array to make sure all elements are still there.
  for (j = 0; j < ARRAY_N; j++) {
    for (i = 0; i < ARRAY_N; i++) {
      if (!(ptr = array_get(a, i))) {
	// Element was not found. We need to check if this element was removed prior to this, otherwise this is an error.
	// We do this by running through u[] up til j
	k = 0;
	for (l = 0; l < j; l++) {
	  if (GET_ID(u[l]) == i) {
	    k = 1;
	    break;
	  }
	}
	if (!k) {
	  // The element was not found in u[0->j], this means the array
	  // has been corrupted.
	  bug("element %d not found after removing %d elements", i, j);
	} else {
	}
      }
    }
//  Now make sure the next element to be removed really is found in the array,
//  then remove it.
    ptr = array_get(a, GET_ID(u[j]));
    if (!ptr) {
      bug("was about to delete element %d from array, but it is already gone!", j);
    }
    array_rm(a, GET_ID(u[j]));
  }
  if (a->elements) {
    // This means that a->elements was not successfully updated, it should be zero by now.
    bug("%zu elements left in array after all were removed", a->elements);
  }
  // Everything seems to check out. Now we need to free all that memory we have malloc'd.
  array_free(a);
  free(a);
  return 1;
}
#endif
