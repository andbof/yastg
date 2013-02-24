#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "log.h"
#include "names.h"
#include "ptrarray.h"
#include "mtrandom.h"
#include "stringtree.h"

void names_init(struct name_list *l)
{
	INIT_LIST_HEAD(&l->taken);
	l->prefix = ptrarray_create();
	l->first  = ptrarray_create();
	l->second = ptrarray_create();
	l->suffix = ptrarray_create();
}

void names_free(struct name_list *l)
{
	st_destroy(&l->taken, ST_DONT_FREE_DATA);
	ptrarray_free(l->prefix);
	ptrarray_free(l->first);
	ptrarray_free(l->second);
	ptrarray_free(l->suffix);
}

void names_load(struct name_list *l, const char * const prefix, const char * const first,
		const char * const second, const char * const suffix)
{
	if (prefix)
		l->prefix = file_to_ptrarray(prefix, l->prefix);
	if (first)
		l->first  = file_to_ptrarray(first, l->first);
	if (second)
		l->second = file_to_ptrarray(second, l->second);
	if (suffix)
		l->suffix = file_to_ptrarray(suffix, l->suffix);
}

static char* create_name(struct name_list *l)
{
	char *pr = NULL;
	char *fi = NULL;
	char *se = NULL;
	char *su = NULL;
	size_t len = 0;

	if (mtrandom_uint(2) == 0) {
		pr = ptrarray_get(l->prefix, mtrandom_uint(l->prefix->used));
		if (pr)
			len += strlen(pr);
	} else {
		su = ptrarray_get(l->suffix, mtrandom_uint(l->suffix->used));
		if (su)
			len += strlen(su);
	}

	fi = ptrarray_get(l->first, mtrandom_uint(l->first->used));
	if (fi)
		len += strlen(fi);

	se = ptrarray_get(l->second, mtrandom_uint(l->second->used));
	if (se)
		len += strlen(se);

	assert(len > 0);	/* Fails if no names are loaded */

	len += 4;		/* Spaces between words and ending null */
	char *name;
	name = malloc(len);
	if (!name)
		return NULL;
	*name = '\0';

	char *p = name;
	if (pr) {
		strcpy(p, pr);
		p += strlen(pr);
		*p = ' ';
		p++;
	}
	if (fi) {
		strcpy(p, fi);
		p += strlen(fi);
		*p = ' ';
		p++;
	}
	if (se) {
		strcpy(p, se);
		p += strlen(se);
		*p = ' ';
		p++;
	}
	if (su) {
		strcpy(p, su);
		p += strlen(su);
		*p = ' ';
		p++;
	}
	assert(*name);		/* Fails if nothing has been added to the string at all */
	if (*(p - 1) == ' ')
		*(p - 1) = '\0';

	return name;
}

char* create_unique_name(struct name_list *l)
{
	char *name = NULL;

	do {
		free(name);
		name = create_name(l);
	} while (st_lookup_exact(&l->taken, name));

	/*
	 * The data pointer in the string tree merely needs to evaluate to true,
	 * because it will never be dereferenced. Using the name string itself
	 * is fine, even though it might not be a valid pointer in the future.
	 */
	if (st_add_string(&l->taken, name, name)) {
		free(name);
		return NULL;
	}

	return name;
}

int is_names_loaded(struct name_list *l)
{
	if (l->prefix->used || l->first->used || l->second->used || l->suffix->used)
		return 1;
	else
		return 0;
}
