#ifndef _HAS_STACK_H
#define _HAS_STACK_H

#include <pthread.h>

struct stack {
	void *datastart;
	void *dataend;
	void *allocend;
	size_t allocd;
	size_t esize;
	pthread_mutex_t mutex;
};

struct stack* stack_init(size_t len, size_t esize);
void stack_push(struct stack *s, void *ptr);
void* stack_qpop(struct stack *s);
void stack_pop(struct stack *s, void *data);
void stack_free(struct stack *s);

#endif
