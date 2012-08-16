#ifndef HAS_ID_H
#define HAS_ID_H

void init_id();
void id_destroy();
unsigned long gen_id();
void insert_id(unsigned long id);
void rm_id(unsigned long id);
int sort_id(void *a, void *b);
int sort_ulong(void *a, void *b);
int sort_dlong(void *a, void *b);
int sort_double(void *a, void *b);

char* hundreths(unsigned long l);

#endif
