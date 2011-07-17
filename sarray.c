#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "sarray.h"
#include "defines.h"
#include "log.h"
#include "sarray.h"
#include "shuffle.h"
#include "id.h"


#define REALLOC_STEP 1	// The base increase for each realloc

/*
 * Initializes a sorted array. Allocates memory and sets initial options.
 * esize is the element size and asize the initial array size.
 */
struct sarray* sarray_init(size_t asize, int maxkey, void (*freefnc)(void*), int (*sortfnc)(void*, void*)) {
  struct sarray *a;
  MALLOC_DIE(a, sizeof(*a));
  a->elements = 0;
  if (asize > 0) {
    MALLOC_DIE(a->array, asize);
    a->allocated = asize;
  } else {
    a->array = NULL;
    a->allocated = 0;
  }
  a->maxkey = maxkey;
  (a->freefnc) = freefnc;
  (a->sortfnc) = sortfnc;
  return a;
}

/*
 * Moves all contents in sorted array b into sarray a. Does not free b but all elements will be removed.
 */
void sarray_move(struct sarray *a, struct sarray *b) {
  size_t l;
  void *ptr;
  for (l = b->elements; l > 0; l--) {
    ptr = sarray_getbypos(b, l-1);
    sarray_add(a, ptr);
    sarray_rmbypos(b, l-1);
  }
}

/*
 * Finds the element indexed by key in the sarray a, using the
 * sorting function defined in the sarray.
 */
void* sarray_getbyid(struct sarray *a, void *key) {
  if (!a->elements) {
    // Array has zero elements
    return NULL;
  } else {
    return sarray_recgetbyid(a, key, 0, a->elements-1);
  }
}

/*
 * Recursively finds the element identified by key in the sarray a.
 * Uses the sorting function defined internally in the sarray.
 */
void* sarray_recgetbyid(struct sarray *a, void *key, size_t l, size_t u) {
  void *half = a->array + l + ((u - l) >> 1);
  if (u < l) {
    // We've ended up with the upper bound lower than the lower, this is probably
    // a rounding error. Element does not exist.
    printf("warning: detected rounding error in sarray_recgetbyid: u=%zu, l=%zu, a@%p\n", u, l, a);
    return NULL;
  } else if (a->sortfnc(key, half) == 0) {
    // We've stumbled upon the element, return it.
    return *((void**)half);
  } else if (u - l < 3) {
    // We only have zero, two or three elements left in the array, check if tha
    // if so, return it. Otherwise return the null pointer.
    if (a->sortfnc(key, a->array + l) == 0) {
      return *(void**)(a->array + l);
    } else if (a->sortfnc(key, a->array + u) == 0) {
      return *(void**)(a->array + u);
    } else {
      return NULL;
    }
  } else if (a->sortfnc(key, half) < 0) {
    // The element is in the lower half of the array
    return sarray_recgetbyid(a, key, l, l+((u-l)>>1));
  } else {
    // The element is in the upper half of the array
    return sarray_recgetbyid(a, key, l+((u-l)>>1), u);
  }
}

/*
 * Returns the lower bound and the number of elements maching key.
 * Only really useful for SARRAY_ALLOW_MULTIPLE sarrays.
 */
struct ptr_num* sarray_getlnbyid(struct sarray *a, void *key) {
  void **ptr;
  struct ptr_num *result;
  MALLOC_DIE(result, sizeof(*result));
  if (!a->elements) {
    result->ptr = NULL;
    result->num = 0;
  } else {
    // Find an element with id key
    if ((ptr = sarray_recgetbyid(a, key, 0, a->elements-1))) {
      // FIXME: This can be more efficient
      // Find the lower bound
      result->ptr = *ptr;
      while ((a->sortfnc(*(void**)result->ptr, key) == 0) && (result->ptr > a->array))
	result->ptr--;
      // Find the number of elements
      while ((a->sortfnc(*ptr, key) == 0) && (*ptr < a->array + a->elements))
	ptr++;
      result->num = *ptr - result->ptr;
    } else {
      result->ptr = NULL;
      result->num = 0;
    }
  }
  return result;
}

/*
 * Finds the previous element to the element specified by e in the sarray a.
 * Assumes: First in each element is an size_t which is the sorting index.
 */
void* sarray_getprevbyid(struct sarray *a, void *key) {
  void *ptr;
  if (!a->elements) {
    // Array has zero elements, return element -1
    // (even though this is outside the memory area)
    return a->array;
  } else {
    ptr = sarray_bubblegetprev(a, key);
    return ptr;
  }
}

void* sarray_bubblegetprev(struct sarray *a, void *key) {
  size_t i = 0;
  while ((i < a->elements) && (a->sortfnc(*(void**)(a->array + i), key) < 0)) {
    i++;
  }
  if ((i == 0) && (a->sortfnc(*(void**)(a->array + i), key) > -1)) {
    // Previous element is actually element -1, return it (even though it is outside the array)
    return *(void**)a->array;
  }
  i--;
  return *(void**)(a->array + i);
}

void* sarray_recgetprev(struct sarray *a, void *key, size_t u, size_t l) {
  bug("%s", "recfindprevelement not implemented"); // FIXME
  return NULL;
}

/*
 * Get element from sarray by number
 */
void* sarray_getbypos(struct sarray *a, size_t n) {
  return a->array + n;
}

/*
 * Inserts an element e into the sarray a.
 */
int sarray_add(struct sarray *a, void *e) {
  void *ptr;
  size_t i;
  if (SIZE_MAX - a->elements < 2) // We reserve an element at the end to be able to loop through the array using an size_t
    die("sarray at %p is full", a);
  if (a->elements == a->allocated) {
    // The last element of the array is used. This means we have to increase the allocated size
    // for the array.
    if (!(ptr = realloc(a->array, a->allocated+REALLOC_STEP))) {
      // We couldn't allocate more memory. Bail out.
      die("realloc to %zu bytes failed", a->allocated+REALLOC_STEP);
    }
    a->array = ptr;
    a->allocated += REALLOC_STEP;
  } else if (a->elements > a->allocated) {
    bug("sarray has more elements than memory allocated: elements = %zu, allocated = %zu", a->elements, a->allocated);
  }
  // We now know there is room in the array for at least one more element.
  // Find where this element fits in
  if ((a->maxkey == SARRAY_ENFORCE_UNIQUE) && ((ptr = sarray_getbyid(a, e)))) {
    // Element already exists, die
    bug("sarray @ %p with SARRAY_ENFORCE_UNIQUE already has element %zx at %p\n", a, *((size_t*)ptr), ptr);
  }
//  printf("FOO: GETPREVBYID called for a = %p, GET_ID(e) = %zx, e@%p\n", a, GET_ID(e), e);
  if (!(ptr = sarray_getprevbyid(a, e))) {
    bug("%s", "something went wrong when trying to find the previous element");
  }
  ptr++;	// ptr now points to the first element to be moved
  MEMMOVE_DIE(ptr + 1, ptr, a->elements - (ptr - a->array));
  // We will now insert the element at the position specified by ptr, since the other
  // data has been moved out of the way
  a->elements++;
  ptr = e;
  // Return 1 since we succeeded
  return 1;
}

void sarray_rmbyid(struct sarray *a, void *key) {
  void *ptr;
  if (!(ptr = sarray_getbyid(a, key))) {
    bug("tried to remove element with id %zx but element was not found", *((size_t*)key));
  }
  MEMMOVE_DIE(ptr, ptr + 1, a->elements-1-(ptr-a->array));
  a->elements--;
}

void sarray_freebyid(struct sarray *a, void *key) {
  void *ptr;
  if (!(ptr = sarray_getbyid(a, key))) {
    bug("tried to remove element with id %zx but element was not found", *((size_t*)key));
  }
  a->freefnc(ptr);
  MEMMOVE_DIE(ptr, ptr + 1, a->elements-1-(ptr-a->array));
  a->elements--;
}

void sarray_rmbypos(struct sarray *a, size_t pos) {
  void *ptr;
  if (!(ptr = sarray_getbypos(a, pos))) {
    bug("tried to remove element %zu but failed", pos);
  }
  MEMMOVE_DIE(ptr, ptr + 1, a->elements-1-(ptr-a->array));
  a->elements--;
}

void sarray_rmbyptr(struct sarray *a, void* ptr) {
  if (ptr > a->array+a->elements)
    bug("tried to remove ptr %p from array %p, but it is outside the array", ptr, a);
  MEMMOVE_DIE(ptr, ptr + 1, a->elements-1-(ptr-a->array));
  a->elements--;
}

void sarray_print(struct sarray *a) {
  void *ptr;
  printf("printarray (%zu elements):", a->elements);
  for (ptr = a->array; ptr < a->array + a->elements; ptr++) {
    printf(" %p", ptr);
  }
  printf("\n");
}

void sarray_free(struct sarray *a) {
  size_t l;
  if (a->freefnc) {
    for (l = 0; l < a->elements; l++) {
      (*(a->freefnc))(a->array+l);
    }
  }
  free(a->array);
}

#ifdef TEST
#define SORTEDARRAY_N 128
/*
 * This function tests the sorted array routines by generating a sorted array,
 * inserting elements into it in a random order making sure the array stays
 * uncorrupted. They are then deleted in another random order, checking for
 * each deletion that the array is uncorrupted.
 */
int sarray_test() {
  struct foo {
    size_t id;
    int gurka;
  } *u[SORTEDARRAY_N];
  void *ptr, *lptr;
  int i, j, k, l;
  // *a will be our test array, allocate memory for it and all its elements.
  // FIXME: Write and use initialization routines for sorted arrays.
  struct sarray *a;
  MALLOC_DIE(a, sizeof(*a));
  a->allocated=0;
  a->elements=0;
  MALLOC_DIE(a->array, sizeof(struct foo)*SORTEDARRAY_N);
  a->allocated=SORTEDARRAY_N;
  memset(a->array, 0, sizeof(struct foo)*SORTEDARRAY_N);
  a->maxkey = SARRAY_ENFORCE_UNIQUE;
  a->sortfnc = &sort_id;
  a->freefnc = NULL;
  // u[] is our array with test elements to add and remove
  for (i = 0; i < SORTEDARRAY_N; i++) {
    MALLOC_DIE(u[i], sizeof(struct foo));
    u[i]->id = i;
  }
  // Randomize u[] to add elements in a random order
  shuffleptr((void**)u, SORTEDARRAY_N);
  // Add all elements in u[] to array a
  for (i = 0; i < SORTEDARRAY_N; i++) {
    sarray_add(a, u[i]);
    lptr = NULL;
    // For each insertion, make sure the array stays sorted
    for (j = 0; j < i; j++) {
      if ((ptr = sarray_getbyid(a, &j))) {
	if ((lptr == NULL) || (ptr == lptr+sizeof(struct foo))) {
	  lptr = ptr;
	} else{
	  bug("element %d found out of order at address %p after adding %d elements. lptr is %p\n", j, ptr, i, lptr);
	}
      }
/*      for (!(ptr = sarray_getbyid(a, *((size_t*)u[j])))) {
	printf("element %d not found after adding %d elements.\n", j, i);
	bug("sarray test failed");
      }*/ // FIXME: Add this type of test
    }
//    printf("insertion test %d passed\n", i);
  }
  // Randomize u[] again before deleting elements
  shuffleptr((void**)u, sizeof(struct foo));
  // Now we delete elements from a in the order they are found in u[].
  // For each deletion, we run through the array to make sure it is uncorrupted
  // and stays sorted.
  for (j = 0; j < SORTEDARRAY_N; j++) {
    lptr = NULL;
    for (i = 0; i < SORTEDARRAY_N; i++) {
      if ((ptr = sarray_getbyid(a, &i))) {
	if ((lptr == NULL) || (ptr == lptr+sizeof(struct foo))) {
	  // Element was found in the correct position. Remember this element.
	  lptr = ptr;
	} else {
	  bug("element %d found out of order at address %p after removing %d elements. lptr is %p\n", i, ptr, j, lptr);
	}
      } else {
	// Element was not found. We need to check if this element was removed prior to this, otherwise this is an error.
	// We do this by running through u[] up til j
	k = 0;
	for (l = 0; l < j; l++) {
	  if (*((size_t*)u[l]) == i) {
	    k = 1;
	    break;
	  }
	}
	if (!k) {
	  // The element was not found in u[0->j], this means the array
	  // has been corrupted.
	  bug("element %d not found after removing %d elements\n", i, j);
	} else {
//	  printf("element %d not found, but it has been removed\n", i);
	}
      }
    }
//  Now make sure the next element to be removed really is found in the array,
//  then remove it.
    ptr = sarray_getbyid(a, ((size_t*)u[j]));
    if (!ptr) {
      bug("element %d was about to be removed but it is already gone!\n", j);
    }
    sarray_rmbyid(a, ((size_t*)u[j]));
  }
  if (a->elements) {
    // This means that a->elements was not successfully updated, it should be zero by now.
    bug("%zu elements left in array after all were removed", a->elements);
  }
  // Everything seems to check out. Now we need to free all that memory we have malloc'd.
  for (i = 0; i < SORTEDARRAY_N; i++) {
    free(u[i]);
  }
  free(a->array);
  free(a);
  return 1;
}
#endif
