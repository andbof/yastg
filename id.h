#ifndef HAS_ID_H
#define HAS_ID_H

void init_id();
size_t gen_id();
void insert_id(size_t id);
void rm_id(size_t id);
int sort_id(void *a, void *b);
int sort_ulong(void *a, void *b);
int sort_dlong(void *a, void *b);
int sort_double(void *a, void *b);

char* hundreths(unsigned long l);

#endif
