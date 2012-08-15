/*
 * Differences from official version available at
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html
 * - Created header file mt19937ar-cok.h
 * - Changed names to mt_* namespace
 * - Fixed coding style to be consistent with project in general
 */

/* 
   A C-program for MT19937, with initialization improved 2002/2/10.
   Coded by Takuji Nishimura and Makoto Matsumoto.
   This is a faster version by taking Shawn Cokus's optimization,
   Matthe Bellew's simplification, Isaku Wada's real version.

   Before using, initialize the mt_state by using mt_init_genrand(seed) 
   or mt_init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The names of its contributors may not be used to endorse or promote 
   products derived from this software without specific prior written 
   permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include <stdio.h>
#include "mt19937ar-cok.h"

/* Period parameters */  
#define MT_N 624
#define MT_M 397
#define MT_MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define MT_UMASK 0x80000000UL /* most significant w-r bits */
#define MT_LMASK 0x7fffffffUL /* least significant r bits */
#define MT_MIXBITS(u,v) ( ((u) & MT_UMASK) | ((v) & MT_LMASK) )
#define MT_TWIST(u,v) ((MT_MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MT_MATRIX_A : 0UL))

static unsigned long mt_state[MT_N]; /* the array for the mt_state vector  */
static int mt_left = 1;
static int mt_initf = 0;
static unsigned long *mt_next;

/* initializes mt_state[MT_N] with a seed */
void mt_init_genrand(unsigned long s)
{
	int j;
	mt_state[0] = s & 0xffffffffUL;
	for (j = 1; j < MT_N; j++) {
		mt_state[j] = (1812433253UL * (mt_state[j - 1] ^ (mt_state[j - 1] >> 30)) + j); 
		/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
		/* In the previous versions, MSBs of the seed affect   */
		/* only MSBs of the array mt_state[].                        */
		/* 2002/01/09 modified by Makoto Matsumoto             */
		mt_state[j] &= 0xffffffffUL;  /* for >32 bit machines */
	}
	mt_left = 1; mt_initf = 1;
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void mt_init_by_array(unsigned long init_key[], int key_length)
{
	int i, j, k;
	mt_init_genrand(19650218UL);
	i = 1; j = 0;
	k = (MT_N > key_length ? MT_N : key_length);
	for (; k; k--) {
		mt_state[i] = (mt_state[i] ^ ((mt_state[i - 1] ^ (mt_state[i - 1] >> 30)) * 1664525UL))
			+ init_key[j] + j; /* non linear */
		mt_state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
		i++; j++;
		if (i>=MT_N) { mt_state[0] = mt_state[MT_N - 1]; i = 1; }
		if (j>=key_length) j=0;
	}
	for (k = MT_N - 1; k; k--) {
		mt_state[i] = (mt_state[i] ^ ((mt_state[i - 1] ^ (mt_state[i - 1] >> 30)) * 1566083941UL))
			- i; /* non linear */
		mt_state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
		i++;
		if (i >= MT_N) { mt_state[0] = mt_state[MT_N - 1]; i = 1; }
	}

	mt_state[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
	mt_left = 1; mt_initf = 1;
}

static void mt_next_state(void)
{
	unsigned long *p=mt_state;
	int j;

	/* if mt_init_genrand() has not been called, */
	/* a default initial seed is used         */
	if (mt_initf == 0) mt_init_genrand(5489UL);

	mt_left = MT_N;
	mt_next = mt_state;

	for (j = MT_N - MT_M + 1; --j; p++) 
		*p = p[MT_M] ^ MT_TWIST(p[0], p[1]);

	for (j = MT_M; --j; p++) 
		*p = p[MT_M - MT_N] ^ MT_TWIST(p[0], p[1]);

	*p = p[MT_M - MT_N] ^ MT_TWIST(p[0], mt_state[0]);
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long mt_genrand_int32(void)
{
	unsigned long y;

	if (--mt_left == 0)
		mt_next_state();
	y = *mt_next++;

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
long mt_genrand_int31(void)
{
	unsigned long y;

	if (--mt_left == 0)
		mt_next_state();
	y = *mt_next++;

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return (long)(y >> 1);
}

/* generates a random number on [0,1]-real-interval */
double mt_genrand_real1(void)
{
	unsigned long y;

	if (--mt_left == 0)
		mt_next_state();
	y = *mt_next++;

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return (double)y * (1.0 / 4294967295.0); 
	/* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double mt_genrand_real2(void)
{
	unsigned long y;

	if (--mt_left == 0)
		mt_next_state();
	y = *mt_next++;

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return (double)y * (1.0 / 4294967296.0); 
	/* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double mt_genrand_real3(void)
{
	unsigned long y;

	if (--mt_left == 0)
		mt_next_state();
	y = *mt_next++;

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return ((double)y + 0.5) * (1.0 / 4294967296.0); 
	/* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double mt_genrand_res53(void)
{
	unsigned long a = mt_genrand_int32() >> 5, b = mt_genrand_int32() >> 6; 
	return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */

#if 0
	int main(void)
	{
		int i;
		unsigned long init[4] = {0x123, 0x234, 0x345, 0x456}, length = 4;
		mt_init_by_array(init, length);

		/* This is an example of initializing by an array.
		   You may use mt_init_genrand(seed) with any 32bit integer
		   as a seed for a simpler initialization */

		printf("1000 outputs of mt_genrand_int32()\n");
		for (i = 0; i < 1000; i++) {
			printf("%10lu ", mt_genrand_int32());
			if (i % 5 == 4)
				printf("\n");
		}

		printf("\n1000 outputs of genrand_real2()\n");
		for (i = 0; i < 1000; i++) {
			printf("%10.8f ", mt_genrand_real2());
			if (i % 5 == 4)
				printf("\n");
		}

		return 0;
	}
#endif
