#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "defines.h"
#include "log.h"
#include "stack.h"

/*
 * Initializes the stack. Not thread safe.
 */
struct stack* stack_init(size_t len, size_t esize)
{
	struct stack *s;
	size_t lensize = len * esize;
	MALLOC_DIE(s, sizeof(*s));
	pthread_mutex_init(&s->mutex, NULL);
	if (esize == 0)
		bug("%s", "illegal element size: 0");
	if (len > 0) {
		MALLOC_DIE(s->datastart, lensize);
		s->allocend = s->datastart + lensize;
		s->dataend = s->datastart;
		s->allocend = s->datastart + lensize;
		s->esize = esize;
	} else {
		s->datastart = s->dataend = s->allocend = NULL;
		s->allocd = 0;
		s->esize = esize;
	}
	return s;
}

/*
 * Increases the size of the stack to two times the current size.
 * Not thread safe!
 */
void stack_grow(struct stack *s)
{
	size_t len;
	if (s->allocend != NULL) {
		len = s->allocend - s->datastart + s->esize;
		REALLOC_DIE(s->datastart, len << 1);
		s->dataend = s->datastart + len - s->esize;
		s->allocend = s->datastart + (len << 1) - s->esize;
	} else {
		MALLOC_DIE(s->datastart, s->esize);
		s->dataend = s->datastart - s->esize;
		s->allocend = s->datastart;
	}
}

/*
 * Decreases the size of the stack to half the current size if possible
 * Not thread safe!
 */
void stack_shrink(struct stack *s)
{
	size_t len;
	/* FIXME: Implement */
}

/*
 * Adds an element to the end of the stack in
 * a thread-safe way.
 */
void stack_push(struct stack *s, void *ptr)
{
	pthread_mutex_lock(&s->mutex);
	if (s->dataend == s->allocend) {
		stack_grow(s);
	}
	s->dataend += s->esize;
	memcpy(s->dataend, ptr, s->esize);
	pthread_mutex_unlock(&s->mutex);
}

/*
 * Quick-pops the stack. This does not lock/unlock the mutex and
 * does not copy the data to another position in memory.
 */
void* stack_qpop(struct stack *s)
{
	void *data;
	data = s->dataend;
	s->dataend -= s->esize;
	return data;
}

/*
 * Pops the stack in a thread-safe way, copying the last element
 * to another position in memory before returning it.
 */
void stack_pop(struct stack *s, void *data)
{
	pthread_mutex_lock(&s->mutex);
	memcpy(data, s->dataend, s->esize);
	s->dataend -= s->esize;
	stack_shrink(s);	/* FIXME: Don't do this if the stack is more than 1/3 full (or something) */
	pthread_mutex_unlock(&s->mutex);
}

void stack_free(struct stack *s)
{
	free(s->datastart);
	free(s);
}

#ifdef TEST

#define STACK_N 8

void stack_test()
{
	int u[STACK_N];
	int v[STACK_N];
	int i;
	struct stack *s = stack_init(0, sizeof(int));
	for (i = 0; i < STACK_N; i++)
		u[i] = i;
	for (i = 0; i < STACK_N; i++)
		stack_push(s, &u[i]);
	for (i = STACK_N-1; i > -1; i--) {
		if (i % 2 == 0)
			stack_pop(s, &v[i]);
		else
			v[i] = *((int*)stack_qpop(s));
	}
	for (i = 0; i < STACK_N; i++) {
		if (u[i] != v[i])
			bug("push'd array doesn't match pop'd array at position %d\n", i);
	}
	stack_free(s);
}

#endif
