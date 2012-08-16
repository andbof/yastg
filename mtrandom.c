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

#if 0
#define RANDOM_INT_FUNCTION(TYPE, NAME)							\
	TYPE mtrandom_ ## NAME(TYPE range)						\
	{										\
		uint32_t ran;								\
		TYPE res = 0;								\
		unsigned int i = 0;							\
		if (range == 0)								\
			return (range)							\
		do {									\
			i += sizeof(int32_t);						\
			ran = mt_genrand_int32();					\
			res |= ran;							\
			if (i < sizeof(TYPE))						\
				res <<= sizeof(int32_t)*8;				\
		} while (i < sizeof(TYPE));						\
		return (TYPE)res % range;						\
	}

RANDOM_INT_FUNCTION(unsigned int, uint);
RANDOM_INT_FUNCTION(int, int);
RANDOM_INT_FUNCTION(unsigned long, ulong);
#endif

unsigned int mtrandom_uint(unsigned int range)
{
	uint32_t ran;
	unsigned int res = 0;
	unsigned int i = 0;
	if (range == 0)
		return 0;
	do {
		i += sizeof(int32_t);
		ran = mt_genrand_int32();
		res |= ran;
		if (i < sizeof(unsigned int))
			res <<= sizeof(int32_t)*8;
	} while (i < sizeof(unsigned int));
	return (unsigned int)res % range;
}

unsigned long mtrandom_ulong(unsigned long range)
{
	uint32_t ran;
	unsigned long res = 0;
	unsigned int i = 0;
	if (range == 0)
		return 0;
	do {
		i += sizeof(int32_t);
		ran = mt_genrand_int32();
		res |= ran;
		if (i < sizeof(unsigned long))
			res <<= sizeof(int32_t)*8;
	} while (i < sizeof(unsigned long));
	return (unsigned long)res % range;
}

int mtrandom_int(int range)
{
	uint32_t ran;
	int res = 0;
	unsigned int i = 0;
	if (range == 0)
		return range;
	do {
		i += sizeof(int32_t);
		ran = mt_genrand_int32();
		res |= ran;
		if (i < sizeof(int))
			res <<= sizeof(int32_t)*8;
	} while (i < sizeof(int));
	return (int)res % range;
}

inline double mtrandom_double(double range)
{
	/* This method will actually generate a numerical distribution error
	   of range * 2^(-32), but it is good enough for our purposes. */
	double r = floor(mt_genrand_real2() * range);
	return r;
}

inline int mtrandom_bool()
{
	return mt_genrand_int32() & 0x1;
}
