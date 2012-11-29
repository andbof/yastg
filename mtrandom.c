#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include "mtrandom.h"
#include "mt19937ar-cok.h"

#define RANDOM_DEV "/dev/urandom"

void mtrandom_init()
{
	FILE *f;
	uint32_t seed;
	unsigned int n;
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

int64_t mtrandom_int64(int64_t range)
{
	int64_t r = 0;
	if (range == 0)
		return range;

	r |= mt_genrand_int32();
	r <<= 32;
	r |= mt_genrand_int32();

	return r % range;
}

uint64_t mtrandom_uint64(uint64_t range)
{
	uint64_t r = 0;
	if (range == 0)
		return range;

	r |= mt_genrand_int32();
	r <<= 32;
	r |= mt_genrand_int32();

	return r % range;
}

unsigned int mtrandom_uint(unsigned int range)
{
	if (range == 0)
		return range;

	return mt_genrand_int32() % range;
}

int mtrandom_int(int range)
{
	if (range == 0)
		return range;

	return mt_genrand_int32() % range;
}

unsigned long mtrandom_ulong(unsigned long range)
{
	return mtrandom_uint64(range);
}

double mtrandom_double(double range)
{
	/* This method will actually generate a numerical distribution error
	   of range * 2^(-32), but it is good enough for our purposes. */
	double r = floor(mt_genrand_real2() * range);
	return r;
}

int mtrandom_bool()
{
	return mt_genrand_int32() & 0x1;
}
