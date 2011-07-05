#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "stable.h"
#include "log.h"

/*
 * DJBs "times 33" hash
 */
unsigned long hash33(char *key) {
  unsigned long hash = 0;
  if (key == NULL)
    bug("%s", "trying to hash a null pointer");
  while (*key) {
    hash += (hash << 5) + *key;
    key++;
  }
  return hash;
}

void* stable_create() {
  struct stable *s;
  MALLOC_DIE(s, sizeof(*s));
  memset(s, '\0', sizeof(*s));
  return s;
}

void* stable_add(struct stable *t, char *key, void *data) {
  unsigned long hash = hash33(key) % STABLE_SIZE;
  struct st_elem *cur = t->table[hash], *next;
  if (cur) {
    next = cur->next;
    while (next != NULL) {
      cur = next;
      next = next->next;
    }
    MALLOC_DIE(cur->next, sizeof(*cur->next));
    next = cur->next;
  } else {
    MALLOC_DIE(t->table[hash], sizeof(*next));
    next = t->table[hash];
    cur = NULL;
  }
  next->data = data;
  next->prev = cur;
  next->next = NULL;
  return next;
}

void* stable_get(struct stable *t, char *key) {
  unsigned long hash = hash33(key) % STABLE_SIZE;
  struct st_elem *elem = t->table[hash];
  while ((elem != NULL) && (strcmp(elem->data, key) != 0))
    elem = elem->next;
  return (void*)elem;
}

void stable_rm(struct stable *t, char *key) {
  unsigned long hash = hash33(key) % STABLE_SIZE;
  struct st_elem *cur = stable_get(t, key), *prev;
  if (cur == NULL)
    bug("string %s does not exist in stable", key);
  prev = cur->prev;
  if (prev == NULL) {
    free(t->table[hash]);
    t->table[hash] = NULL;
  } else {
    prev->next = cur->next;
    if (cur->next)
      cur->next->prev = prev;
    free(cur);
  }
}

void stable_free(struct stable *t) {
  unsigned long l;
  struct st_elem *cur;
  struct st_elem *next;
  for (l = 0; l < STABLE_SIZE; l++) {
    cur = t->table[l];
    while (cur) {
      next = cur->next;
      free(cur);
      cur = next;
    }
  }
  free(t);
}
