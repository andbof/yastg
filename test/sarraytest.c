#include <stdio.h>
#include <limits.h>
#include <config.h>
#include "common.h"
#include "sarray.h"
#include "mtrandom.h"

struct foo {
	unsigned long id;
	int gurka;
};

/* Shamelessly taken from
 * http://benpfaff.org/writings/clc/shuffle.html
 *
 * Arrange the N elements of ARRAY in random order.
 * Only effective if N is much smaller than RAND_MAX;
 * if this may not be the case, use a better random
 * number generator. */
void shuffleptr(struct foo **array, unsigned int n)
{
	unsigned int i, j;
	void* k;
	if (n < 2)
		return;

	for (i = 0; i < n - 1; i++) {
		j = i + mtrandom_uint(UINT_MAX) / (UINT_MAX / (n - i) + 1);
		k = array[j];
		array[j] = array[i];
		array[i] = k;
	}
}

#define SORTEDARRAY_N 8
/*
 * This function tests the sorted array routines by generating a sorted array,
 * inserting elements into it in a random order making sure the array stays
 * uncorrupted. They are then deleted in another random order, checking for
 * each deletion that the array is uncorrupted.
 */
int main(int argc, char **argv)
{
	struct foo *u[SORTEDARRAY_N];
	void *ptr, *lptr;
	long i, j, k, l;
	/* *a will be our test array, allocate memory for it and all its elements. */
	/* FIXME: Write and use initialization routines for sorted arrays. */
	struct sarray *a;

	a = malloc(sizeof(*a));
	a->allocated=0;
	a->elements=0;
	a->element_size = sizeof(struct foo);
	a->array = malloc(sizeof(struct foo)*SORTEDARRAY_N);
	a->allocated = SORTEDARRAY_N;
	a->array = malloc(sizeof(struct foo)*SORTEDARRAY_N);
	a->maxkey = SARRAY_ENFORCE_UNIQUE;
	a->sortfnc = &sort_ulong;
	a->freefnc = NULL;

	/* u[] is our array with test elements to add and remove */
	for (i = 0; i < SORTEDARRAY_N; i++) {
		u[i] = malloc(sizeof(struct foo));
		u[i]->id = i;
	}

	/* Randomize u[] to add elements in a random order */
	shuffleptr(u, SORTEDARRAY_N);

	/* Add all elements in u[] to array a */
	for (i = 0; i < SORTEDARRAY_N; i++) {
		sarray_add(a, u[i]);
		lptr = NULL;
		/* For each insertion, make sure the array stays sorted */
		for (j = 0; j < i; j++) {
			if ((ptr = sarray_getbyid(a, &j))) {
				if ((lptr == NULL) || (ptr == lptr+sizeof(struct foo)))
					lptr = ptr;
				else {
					printf("sarraytest: element %ld found out of order at address %p after adding %ld elements. lptr is %p\n", j, ptr, i, lptr);
					return 5;
				}
			}
		}
	}

	/* Randomize u[] again before deleting elements */
	shuffleptr(u, SORTEDARRAY_N);

	/* Now we delete elements from a in the order they are found in u[].
	   For each deletion, we run through the array to make sure it is uncorrupted
	   and stays sorted. */
	for (j = 0; j < SORTEDARRAY_N; j++) {
		lptr = NULL;
		for (i = 0; i < SORTEDARRAY_N; i++) {
			if ((ptr = sarray_getbyid(a, &i))) {
				if ((lptr == NULL) || (ptr == lptr + sizeof(struct foo))) {
					/* Element was found in the correct position. Remember this element. */
					lptr = ptr;
				} else {
					printf("sarraytest: element %ld found out of order at address %p after removing %ld elements. lptr is %p\n", i, ptr, j, lptr);
					return 1;
				}
			} else {
				/* Element was not found. We need to check if this element was removed prior to this, otherwise this is an error.
				 We do this by running through u[] up til j */
				k = 0;
				for (l = 0; l < j; l++) {
					if (*((unsigned long*)u[l]) == i) {
						k = 1;
						break;
					}
				}
				if (!k) {
					/* The element was not found in u[0->j], this means the array
					   has been corrupted. */
					printf("sarraytest: element %ld not found after removing %ld elements\n", i, j);
					return 2;
				}
			}
		}
		/*  Now make sure the next element to be removed really is found in the array,
		    then remove it. */
		ptr = sarray_getbyid(a, ((unsigned long*)u[j]));
		if (!ptr) {
			printf("sarraytest: element %ld was about to be removed but it is already gone!\n", j);
			return 3;
		}
		sarray_rmbyid(a, ((unsigned long*)u[j]));
	}

	if (a->elements) {
		/* This means that a->elements was not successfully updated, it should be zero by now. */
		printf("sarraytest: %zu elements left in array after all were removed\n", a->elements);
		return 4;
	}

	/* Everything seems to check out. Clean up. */
	for (i = 0; i < SORTEDARRAY_N; i++)
		free(u[i]);
	free(a->array);
	free(a);

	return 0;
}
