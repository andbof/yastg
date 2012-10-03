#include <stdio.h>
#include <config.h>
#include "common.h"
#include "stack.h"
#include "mtrandom.h"

#define STACK_N 8

int main(int argc, char **argv)
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
		if (u[i] != v[i]) {
			printf("stacktest: push'd array doesn't match pop'd array at position %d\n", i);
			return 1;
		}
	}

	stack_free(s);

	return 0;
}
