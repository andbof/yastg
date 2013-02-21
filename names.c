#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include "common.h"
#include "log.h"
#include "names.h"
#include "ptrarray.h"
#include "mtrandom.h"

void names_init(struct name_list *l)
{
	l->prefix = ptrarray_create();
	l->first  = ptrarray_create();
	l->second = ptrarray_create();
	l->suffix = ptrarray_create();
}

void names_free(struct name_list *l)
{
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

char* create_unique_name(struct name_list *l)
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
	if (*(p - 1) == ' ')
		*(p - 1) = '\0';

	return name;
}
