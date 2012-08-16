#ifndef HAS_MTRANDOM_H
#define HAS_MTRANDOM_H

void mtrandom_init();

unsigned int mtrandom_uint(unsigned int range);
unsigned long mtrandom_ulong(unsigned long range);
double mtrandom_double(double range);
int mtrandom_bool();

#endif
