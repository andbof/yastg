#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include "mtrandom.h"
#include "mt19937ar-cok.h"

#define RANDOM_DEV "/dev/urandom"

void mtrandom_init() {
  FILE *f;
  unsigned long seed;
  int n;
  if (!(f = fopen(RANDOM_DEV, "r"))) {
    printf("No %s, initializing PRNG from system time\n", RANDOM_DEV);
    seed = (unsigned long)time(0);
  } else {
    printf("%s detected, will use for PRNG initialization\n", RANDOM_DEV);
    n = 0;
    while (n < sizeof(seed))
      n += fread(&seed + n, 1, sizeof(seed) - n, f);
    fclose(f);
  }
  mt_init_genrand(seed);
}

inline unsigned int mtrandom_uint(unsigned int range) {
  unsigned long r = mt_genrand_int32() % range;
  return (unsigned int)r;
}

inline unsigned long mtrandom_ulong(unsigned long range) {
  assert(sizeof(unsigned long) <= 4);
  unsigned long r = mt_genrand_int32() % range;
  return r;
}

inline double mtrandom_double(double range) {
  // This method will actually generate a numerical distribution error
  // of range * 2^(-32), but it is good enough for our purposes.
  double r = floor(mt_genrand_real2() * range);
  return r;
}

inline size_t mtrandom_sizet(size_t range) {
  unsigned long r = mt_genrand_int32() % range;
  return (size_t)r;
}

