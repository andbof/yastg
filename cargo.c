#include <stdlib.h>
#include <stdio.h>
#include "cargo.h"

void cargo_init(struct cargo *cargo)
{
	memset(cargo, 0, sizeof(*cargo));
	ptrlist_init(&cargo->requires);
	INIT_LIST_HEAD(&cargo->list);
}

void cargo_free(struct cargo *cargo)
{
	ptrlist_free(&cargo->requires);
}
