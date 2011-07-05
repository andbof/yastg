#ifndef HAS_STABLE_H
#define HAS_STABLE_H

#define STABLE_SIZE 65536

struct stable {
  struct st_elem* table[STABLE_SIZE];
  unsigned long elements;
};

struct st_elem {
  void *data;
  struct st_elem *prev, *next;
};

void* stable_create();
void* stable_add(struct stable *t, char *key, void *data);
void* stable_get(struct stable *t, char *key);
void stable_rm(struct stable *t, char *key);
void stable_free(struct stable *t);

#endif
