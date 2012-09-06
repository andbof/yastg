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
	FILE *f;
	off_t size;
	char *begin, *ptr, *end;

	if ((f = fopen(fn, "r")) == NULL)
		goto load_return;

	if (flock(fileno(f), LOCK_SH) != 0)
		goto load_close;

	if (fseek(f, 0, SEEK_END) != 0)
		goto load_close;
	
	if ((size = ftell(f)) < 0)
		goto load_close;

	if (fseek(f, 0, SEEK_SET) != 0)
		goto load_close;

	if ((begin = mmap(NULL, size, PROT_READ,
			MAP_SHARED | MAP_POPULATE, fileno(f), 0)) == NULL)
		goto load_close;

	end = begin + size;
	ptr = begin;

	unsigned int len;
	char *name, *name_end;
	while (ptr < end) {
		while (ptr <= end && isspace(*ptr))
			ptr++;
		name_end = ptr;
		while (name_end <= end && *name_end != '\n' && *name_end != '#')
			name_end++;

		len = name_end - ptr;
		if (len > 0) {
			name = malloc(len + 1);
			if (name == NULL)
				goto load_munmap;
			memcpy(name, ptr, len);
			while (isspace(name[len - 1]))
				len--;
			name[len] = '\0';
			a = ptrarray_add(a, name);
		}

		/* Skip any comment following */
		while (*name_end != '\n' && name_end <= end)
			name_end++;

		ptr = name_end + 1;
	}

load_munmap:
	munmap(ptr, size);

load_close:
	fclose(f);

load_return:
	return a;
}
