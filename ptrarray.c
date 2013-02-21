#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "list.h"
#include "parseconfig.h"
#include "ptrarray.h"

struct ptrarray* ptrarray_create()
{
	struct ptrarray *a = malloc(sizeof(*a));
	memset(a, 0, sizeof(*a));
	return a;
}

void ptrarray_free(struct ptrarray *a)
{
	assert(a);
	for (size_t i = 0; i < a->used; i++)
		free(a->array[i]);
	free(a);
}

#define PTRARRAY_MIN_ALLOC 16
struct ptrarray* ptrarray_add(struct ptrarray *a, void *ptr)
{
	struct ptrarray *n;
	assert(a->used <= a->alloc);
	if (a->used == a->alloc) {
		if (a->alloc == 0)
			a->alloc = PTRARRAY_MIN_ALLOC / 2;
		n = realloc(a, (a->alloc * 2)*sizeof(void*) + sizeof(*a));
		if (n == NULL)		/* FIXME, this should probably be detected by the caller */
			return a;
		a = n;
		a->alloc *= 2;
	}
	a->array[a->used++] = ptr;
	return a;
}

struct ptrarray* file_to_ptrarray(const char * const fn, struct ptrarray *a)
{
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	struct config *conf;
	char *name;

	if (parse_config_file(fn, &conf_root))
		return NULL;

	list_for_each_entry(conf, &conf_root, list) {
		name = strdup(conf->key);
		if (!name) {
			destroy_config(&conf_root);
			return NULL;
		}
		a = ptrarray_add(a, name);
	}

	destroy_config(&conf_root);
	return a;
}

void* ptrarray_get(struct ptrarray *a, unsigned int idx) {
	if (idx >= a->used)
		return NULL;

	return a->array[idx];
}
