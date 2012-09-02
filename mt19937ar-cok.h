#ifndef _MT19937AR_COK_H
#define _MT19937AR_COK_H

#include <stdint.h>

void mt_init_genrand(uint32_t s);
void mt_init_by_array(uint32_t init_key[], int key_length);
uint32_t mt_genrand_int32(void);
int32_t mt_genrand_int31(void);
double mt_genrand_real1(void);
double mt_genrand_real2(void);
double mt_genrand_real3(void);
double mt_genrand_res53(void);

#endif
