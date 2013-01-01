#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "base.h"
#include "cargo.h"
#include "common.h"
#include "stringtree.h"
#include "item.h"
#include "log.h"
#include "planet.h"
#include "planet_type.h"
#include "universe.h"

void base_free(struct base *b)
{
	if (b->name) {
		st_rm_string(&univ.basenames, b->name);
		free(b->name);
	}

	struct cargo *c, *_c;
	list_for_each_entry_safe(c, _c, &b->items, list) {
		list_del(&c->list);
		cargo_free(c);
		free(c);
	}

	pthread_rwlock_destroy(&b->items_lock);
	st_destroy(&b->item_names, ST_DONT_FREE_DATA);
	ptrlist_free(&b->players);
	free(b);
}

static void base_init(struct base *base)
{
	memset(base, 0, sizeof(*base));
	INIT_LIST_HEAD(&base->items);
	pthread_rwlock_init(&base->items_lock, NULL);
	INIT_LIST_HEAD(&base->item_names);
	ptrlist_init(&base->players);
}

#define BASE_CARGO_RANDOMNESS 0.5
static int base_genesis(struct base *base, struct planet *planet)
{
	unsigned int i, j;

	base->planet = planet;
	base->type = ptrlist_random(&planet->type->base_types);
	base->docks = 1; /* FIXME */

	struct cargo *bt_cargo, *cargo, *req;
	struct list_head *lh;
	list_for_each_entry(bt_cargo, &base->type->items, list) {
		cargo = malloc(sizeof(*cargo));
		if (!cargo)
			goto err;
		cargo_init(cargo);

		cargo->item = bt_cargo->item;
		cargo->max = bt_cargo->max * (1 - BASE_CARGO_RANDOMNESS)
			+ mtrandom_ulong(bt_cargo->max * BASE_CARGO_RANDOMNESS * 2);
		cargo->daily_change = bt_cargo->daily_change * (1 - BASE_CARGO_RANDOMNESS)
			+ mtrandom_long(bt_cargo->daily_change * BASE_CARGO_RANDOMNESS * 2);

		cargo->amount = mtrandom_ulong(cargo->max);
		if (cargo->amount > 10)
			cargo->amount = pow(5, log10(cargo->amount));

		if (st_add_string(&base->item_names, cargo->item->name, cargo))
			goto err;

		list_add(&cargo->list, &base->items);
	}

	/*
	 * We can't copy the requirement lists before all the base items are constructed
	 * and registered in the string tree or we wouldn't be able to look them up.
	 */
	list_for_each_entry(bt_cargo, &base->type->items, list) {
		cargo = st_lookup_string(&base->item_names, bt_cargo->item->name);
		ptrlist_for_each_entry(req, &bt_cargo->requires, lh)
			ptrlist_push(&cargo->requires, st_lookup_string(&base->item_names, req->item->name));
	}

	/* FIXME: limit loop */
	do {
		free(base->name);
		base->name = create_unique_name(&univ.avail_base_names);
	} while (st_lookup_exact(&univ.basenames, base->name));

	list_add(&base->list, &univ.bases);

	return 0;

err:
	base_free(base);
	return -1;
}

#define BASE_MAXNUM 3
#define BASE_MUL_ODDS 2
void base_populate_planet(struct planet* planet)
{
	struct base *b;
	int num;

	num = 0;

	if (ptrlist_len(&planet->type->base_types) > 0) {
		while (num < BASE_MAXNUM && mtrandom_uint(UINT_MAX) < UINT_MAX / BASE_MUL_ODDS)
			num++;
	} else {
		num = 0;
	}

	pthread_rwlock_wrlock(&univ.basenames_lock);

	for (int i = 0; i < num; i++) {
		b = malloc(sizeof(*b));
		if (!b)
			goto unlock;
		base_init(b);
		if (base_genesis(b, planet)) {
			free(b);
			goto unlock;
		}
		ptrlist_push(&planet->bases, b);
		st_add_string(&univ.basenames, b->name, b);
	}

unlock:
	pthread_rwlock_unlock(&univ.basenames_lock);
}
