#include <stdio.h>
#include <stdint.h>
#include "defines.h"
#include "id.h"
#include "sarray.h"
#include "limits.h"

struct sarray *id_array;

void init_id() {
  id_array = sarray_init(sizeof(size_t), 0, SARRAY_ENFORCE_UNIQUE, &free, &sort_id);
}

size_t gen_id() {
  size_t i;
  void *ptr;
  do {
    i = mtrandom_sizet(SIZE_MAX); // FIXME should be hash
    ptr = sarray_getbyid(id_array, &i);
  } while (ptr || i == 0);
  return i;
}

void insert_id(size_t id) {
  sarray_add(id_array, &id);
}

void rm_id(size_t id) {
  sarray_rmbypos(id_array, id);
}

char* hundreths(unsigned long l) {
  int mod = l%100;
  char* result;
  MALLOC_DIE(result, 10);	// FIXME: Memory leak
  if (mod < 10) {
    sprintf(result, "%lu.0%lu", l/100, l%100);
  } else {
    sprintf(result, "%lu.0%lu", l/100, l%100);
  }
  return result;
}

/*
 * Sorts based on IDs.
 * Will return neg if b is larger than a, 0 if they're equal and pos if
 * a is larger than b;
 */
int sort_id(void *a, void *b) {
  if (GET_ID(b) > GET_ID(a)) {
    return -1;
  } else if (GET_ID(b) < GET_ID(a)) {
    return 1;
  } else {
    return 0;
  }
//  return GET_ID(a) - GET_ID(b);
}

/*
 * Sorts based on assuming a and b points to unsigned longs.
 */
int sort_ulong(void *a, void *b) {
  if (GET_ULONG(b) > GET_ULONG(a)) {
    return -1;
  } else if (GET_ULONG(b) < GET_ULONG(a)) {
    return 1;
  } else {
    return 0;
  }
//  return GET_ULONG(a) - GET_ULONG(b);
}

/*
 * Sorts based on assuming a and b points to signed longs.
 
int sort_dlong(void *a, void *b) {
  if (GET_DLONG(b) > GET_DLONG(a)) {
    return -1;
  } else if (GET_DLONG(b) < GET_DLONG(a)) {
    return 1;
  } else {
    return 0;
  }
}
*/

int sort_double(void *a, void *b) {
  if (GET_DOUBLE(a) > GET_DOUBLE(b)) {
    return -1;
  } else if (GET_DOUBLE(b) < GET_DOUBLE(a)) {
    return 1;
  } else {
    return 0;
  }
//  return GET_DOUBLE(a) - GET_DOUBLE(b);
}
