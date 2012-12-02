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
int sarray_init(struct sarray *a, unsigned long esize, unsigned long asize, int maxkey, void (*freefnc)(void*), int (*sortfnc)(void*, void*))
{
	a->elements = 0;
	a->element_size = esize;
	if (asize > 0) {
		a->array = malloc(asize * esize);
		if (!a->array)
			return -1;

		a->allocated = asize;
	} else {
		a->array = NULL;
		a->allocated = 0;
	}
	a->maxkey = maxkey;
	(a->freefnc) = freefnc;
	(a->sortfnc) = sortfnc;

	return 0;
}

static void* sarray_bubblegetprev(struct sarray *a, void *key)
{
	unsigned long i = 0;
	while ((i < a->elements) && (a->sortfnc(a->array+i*a->element_size, key) < 0))
		i++;
	if ((i == 0) && (a->sortfnc(a->array+i*a->element_size, key) > -1)) {
		/* Previous element is actually element -1, return it (even though it is outside the array) */
		return a->array - a->element_size;
	}
	i--;
	return a->array+i*a->element_size;
}

/*
 * Finds the previous element to the element specified by e in the sarray a.
 * Assumes: First in each element is an unsigned long which is the sorting index.
 * If the array has zero elements, element -1 will be returned (even though
 * it is outside the valid memory area)
 */
static void* sarray_getprevbyid(struct sarray *a, void *key)
{
	void *ptr;
	if (!a->elements) {
		return (a->array-a->element_size);
	} else {
		ptr = sarray_bubblegetprev(a, key);
		return ptr;
	}
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
	size_t len;

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
	if (a->maxkey == SARRAY_ENFORCE_UNIQUE)
		bug("%s", "SARRAY_ENFORCE_UNIQUE no longer supported");
	if (!(ptr = sarray_getprevbyid(a, e)))
		bug("%s", "something went wrong when trying to find the previous element");

	ptr += a->element_size;	/* ptr now points to the first element to be moved */
	len = a->elements * a->element_size - (ptr - a->array);
	if (!memmove(ptr + a->element_size, ptr, len))
		bug("%s", "memmove failed in sarray_add()");

	/* We will now insert the element at the position specified by ptr, since the other
	   data has been moved out of the way */
	a->elements++;
	memcpy(ptr, e, a->element_size);

	return 1;
}

int sarray_rmbypos(struct sarray *a, unsigned long pos) {
	void *ptr;
	size_t len;

	if (!(ptr = sarray_getbypos(a, pos)))
		bug("tried to remove element %zu but failed", pos);

	len = (a->elements - 1) * a->element_size - (ptr - a->array);
	if (!(memmove(ptr, ptr+a->element_size, len)))
		return -1;

	a->elements--;
	return 0;
}

void sarray_free(struct sarray *a) {
	unsigned long l;
	if (a->freefnc) {
		for (l = 0; l < a->elements; l++)
			(*(a->freefnc))(a->array+l*a->element_size);
	}
	free(a->array);
}

int sort_ulong(void *a, void *b)
{
	if (GET_ULONG(b) > GET_ULONG(a))
		return -1;
	else if (GET_ULONG(b) < GET_ULONG(a))
		return 1;
	else
		return 0;
}

int sort_double(void *a, void *b)
{
	if (GET_DOUBLE(a) > GET_DOUBLE(b))
		return -1;
	else if (GET_DOUBLE(b) < GET_DOUBLE(a))
		return 1;
	else
		return 0;
}
