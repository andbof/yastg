#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include "defines.h"
#include "mtrandom.h"

/* Shamelessly taken from
 * http://benpfaff.org/writings/clc/shuffle.html
 *
 * Arrange the N elements of ARRAY in random order.
 * Only effective if N is much smaller than RAND_MAX;
 * if this may not be the case, use a better random
 * number generator. */
void shuffleptr(void* *array, unsigned int n)
{
	size_t i, j;
	void* k;
	if (n > 1) {
		for (i = 0; i < n - 1; i++) {
			j = i + mtrandom_uint(UINT_MAX) / (UINT_MAX / (n - i) + 1);
			k = array[j];
			array[j] = array[i];
			array[i] = k;
		}
	}
}
