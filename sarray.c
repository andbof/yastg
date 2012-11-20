#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "sarray.h"
#include "common.h"
#include "log.h"
#include "sarray.h"


#define REALLOC_STEP 1	/* The base increase for each realloc */

/*
 * Initializes a sorted array. Allocates memory and sets initial options.
 * esize is the element size and asize the initial array size.
 */
struct sarray* sarray_init(unsigned long esize, unsigned long asize, int maxkey, void (*freefnc)(void*), int (*sortfnc)(void*, void*))
{
	struct sarray *a;
	MALLOC_DIE(a, sizeof(*a));
	a->elements = 0;
	a->element_size = esize;
	if (asize > 0) {
		MALLOC_DIE(a->array, asize*esize);
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
void sarray_move(struct sarray *a, struct sarray *b)
{
	unsigned long l;
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
void* sarray_getbyid(struct sarray *a, void *key)
{
	void *ptr;
	if (!a->elements) {
		return NULL;
	} else {
		ptr = sarray_recgetbyid(a, key, 0, a->elements-1);
		return ptr;
	}
}

void* sarray_bubblegetbyid(struct sarray *a, void *key)
{
	unsigned long i = 0;
	while (i < a->elements*a->element_size && a->sortfnc(a->array+i, key) > 0)
		i += a->element_size;
	if (i >= a->elements*a->element_size) {
		/* Element was not found */
		return NULL;
	} else if (a->sortfnc(key, a->array+i) == 0) {
		return (a->array+i);
	} else {
		/* Element was not found */
		return NULL;
	}
}

/*
 * Recursively finds the element identified by key in the sarray a.
 * Uses the sorting function defined internally in the sarray.
 */
void* sarray_recgetbyid(struct sarray *a, void *key, unsigned long l, unsigned long u)
{
	void *half = a->array+l*a->element_size+((u-l)>>1)*a->element_size;
	if (u < l) {
		/* We've ended up with the upper bound lower than the lower, this is probably
		   a rounding error. Element does not exist. */
		printf("warning: detected rounding error in sarray_recgetbyid: u=%zu, l=%zu, a@%p\n", u, l, a);
		return NULL;
	} else if (a->sortfnc(key, half) == 0) {
		/* We've stumbled upon the element, return it. */
		return (void*)half;
	} else if (u - l < 3) {
		/* We only have zero, two or three elements left in the array, check if tha
		   if so, return it. Otherwise return the null pointer. */
		if (a->sortfnc(key, a->array+l*a->element_size) == 0) {
			return a->array+l*a->element_size;
		} else if (a->sortfnc(key, a->array+u*a->element_size) == 0) {
			return a->array+u*a->element_size;
		} else {
			return NULL;
		}
	} else if (a->sortfnc(key, half) < 0) {
		/* The element is in the lower half of the array */
		return sarray_recgetbyid(a, key, l, l+((u-l)>>1));
	} else {
		/* The element is in the upper half of the array */
		return sarray_recgetbyid(a, key, l+((u-l)>>1), u);
	}
}

/*
 * Returns the lower bound and the number of elements maching key.
 * Only really useful for SARRAY_ALLOW_MULTIPLE sarrays.
 */
struct ptr_num* sarray_getlnbyid(struct sarray *a, void *key)
{
	void* ptr;
	struct ptr_num *result;
	MALLOC_DIE(result, sizeof(*result));
	if (!a->elements) {
		result->ptr = NULL;
		result->num = 0;
	} else {
		/* Find an element with id key */
		if ((ptr = sarray_recgetbyid(a, key, 0, a->elements-1))) {
			/* FIXME: This can be more efficient
			    Find the lower bound */
			result->ptr = ptr;
			while ((a->sortfnc(result->ptr, key) == 0) && (result->ptr > a->array))
				result->ptr--;
			/* Find the number of elements */
			while ((a->sortfnc(ptr, key) == 0) && (ptr < a->array+a->elements*a->element_size))
				ptr++;
			result->num = (ptr - result->ptr)/(a->element_size);
		} else {
			result->ptr = NULL;
			result->num = 0;
		}
	}
	return result;
}

/*
 * Finds the previous element to the element specified by e in the sarray a.
 * Assumes: First in each element is an unsigned long which is the sorting index.
 * If the array has zero elements, element -1 will be returned (even though
 * it is outside the valid memory area)
 */
void* sarray_getprevbyid(struct sarray *a, void *key)
{
	void *ptr;
	if (!a->elements) {
		return (a->array-a->element_size);
	} else {
		/* printf("FOO: CALLING bubblegetprev for (a, key) = (%p, %zx) (key@%p)\n", a, key, &key); */
		ptr = sarray_bubblegetprev(a, key);
		return ptr;
	}
}

void* sarray_bubblegetprev(struct sarray *a, void *key)
{
	unsigned long i = 0;
	/* printf("i = %zd, a->element_size=%zu, a->array = %p, a->elements=%zu, &key=%p, a->sortfnc@%p\n", i, a->element_size, a->array, a->elements, key, a->sortfnc); */
	while ((i < a->elements) && (a->sortfnc(a->array+i*a->element_size, key) < 0))
		i++;
	if ((i == 0) && (a->sortfnc(a->array+i*a->element_size, key) > -1)) {
		/* Previous element is actually element -1, return it (even though it is outside the array) */
		return a->array - a->element_size;
	}
	i--;
	return a->array+i*a->element_size;
}

void* sarray_recgetprev(struct sarray *a, void *key, unsigned long u, unsigned long l)
{
	bug("%s", "recfindprevelement not implemented"); /* FIXME */
	return NULL;
}

/*
 * Get element from sarray by number
 */
void* sarray_getbypos(struct sarray *a, unsigned long n)
{
	return (a->array+n*a->element_size);
}

/*
 * Inserts an element e into the sarray a.
 */
int sarray_add(struct sarray *a, void *e)
{
	void *ptr;
	if (SIZE_MAX - a->elements < 2)
		/* We reserve an element at the end to be able to loop through the array using a unsigned long */
		die("sarray at %p is full", a);

	if (a->elements == a->allocated) {
		/* The last element of the array is used. This means we have to increase the allocated size
		   for the array. */
		if (!(ptr = realloc(a->array, (a->allocated+REALLOC_STEP)*a->element_size)))
			die("realloc to %zu bytes failed", (a->allocated+REALLOC_STEP)*a->element_size);
		a->array = ptr;
		a->allocated += REALLOC_STEP;
	} else if (a->elements > a->allocated) {
		bug("sarray has more elements than memory allocated: elements = %zu, allocated = %zu", a->elements, a->allocated);
	}

	/* We now know there is room in the array for at least one more element.
	   Find where this element fits in */
	if ((a->maxkey == SARRAY_ENFORCE_UNIQUE) && ((ptr = sarray_getbyid(a, e))))
		bug("sarray @ %p with SARRAY_ENFORCE_UNIQUE already has element %zx at %p\n", a, *((unsigned long*)ptr), ptr);
	if (!(ptr = sarray_getprevbyid(a, e)))
		bug("%s", "something went wrong when trying to find the previous element");

	ptr += a->element_size;	/* ptr now points to the first element to be moved */
	MEMMOVE_DIE(ptr+a->element_size, ptr, a->elements*a->element_size-(ptr-a->array));

	/* We will now insert the element at the position specified by ptr, since the other
	   data has been moved out of the way */
	a->elements++;
	memcpy(ptr, e, a->element_size);

	return 1;
}

void sarray_rmbyid(struct sarray *a, void *key)
{
	void *ptr;
	if (!(ptr = sarray_getbyid(a, key)))
		bug("tried to remove element with id %zx but element was not found", *((unsigned long*)key));
	MEMMOVE_DIE(ptr, ptr+a->element_size, (a->elements-1)*a->element_size-(ptr-a->array));
	a->elements--;
}

void sarray_freebyid(struct sarray *a, void *key) {
	void *ptr;
	if (!(ptr = sarray_getbyid(a, key)))
		bug("tried to remove element with id %zx but element was not found", *((unsigned long*)key));
	a->freefnc(ptr);
	MEMMOVE_DIE(ptr, ptr+a->element_size, (a->elements-1)*a->element_size-(ptr-a->array));
	a->elements--;
}

void sarray_rmbypos(struct sarray *a, unsigned long pos) {
	void *ptr;
	if (!(ptr = sarray_getbypos(a, pos)))
		bug("tried to remove element %zu but failed", pos);
	MEMMOVE_DIE(ptr, ptr+a->element_size, (a->elements-1)*a->element_size-(ptr-a->array));
	a->elements--;
}

void sarray_rmbyptr(struct sarray *a, void* ptr) {
	if (ptr > a->array+a->elements*a->element_size)
		bug("tried to remove ptr %p from array %p, but it is outside the array", ptr, a);
	MEMMOVE_DIE(ptr, ptr+a->element_size, (a->elements-1)*a->element_size-(ptr-a->array));
	a->elements--;
}

void sarray_print(struct sarray *a) {
	unsigned long i;
	printf("printarray (%zu elements):", a->elements);
	for (i = 0; i < a->elements; i++)
		printf(" %zu", GET_ID(a->array+i*a->element_size));
	printf("\n");
}

void sarray_free(struct sarray *a) {
	unsigned long l;
	if (a->freefnc) {
		for (l = 0; l < a->elements; l++)
			(*(a->freefnc))(a->array+l*a->element_size);
	}
	free(a->array);
}
