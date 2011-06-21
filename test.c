#include <stdio.h>
#include "test.h"
#include "sarray.h"
#include "array.h"
#include "stack.h"

/*
 * Run self tests. We assume the self test will call exit(n) if failed, where n != 0.
 */
void run_tests() {
#ifdef TEST
  printf("Running self tests ...\n");
  printf("  sarray: ");
  sarray_test();
  printf("passed\n");
  printf("  array: ");
  array_test();
  printf("passed\n");
  printf("  stack: ");
  stack_test();
  printf("passed\n");
  printf("All self tests passed!\n");
#endif
}
