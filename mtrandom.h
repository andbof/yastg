#ifndef _HAS_MTRANDOM_H
#define _HAS_MTRANDOM_H

#include <stdint.h>

void mtrandom_init();

int64_t mtrandom_int64(int64_t range);
uint64_t mtrandom_uint64(uint64_t range);
unsigned int mtrandom_uint(unsigned int range);
int mtrandom_int(int range);
unsigned long mtrandom_ulong(unsigned long range);
double mtrandom_double(double range);

#endif
