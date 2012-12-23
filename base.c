#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "base.h"
#include "common.h"
#include "stringtree.h"
#include "item.h"
#include "log.h"
#include "planet.h"
#include "planet_type.h"
#include "universe.h"

static void base_item_init(struct base_item *item)
{
	memset(item, 0, sizeof(*item));
}

static void base_item_free(struct base_item *item)
{
	return;
}

void base_free(struct base *b)
{
	if (b->name) {
		st_rm_string(&univ.basenames, b->name);
		free(b->name);
	}

	struct base_item *bt, *_bt;
	list_for_each_entry_safe(bt, _bt, &b->items, list) {
		list_del(&bt->list);
		base_item_free(bt);
		free(bt);
	}

	pthread_rwlock_destroy(&b->items_lock);
	ptrlist_free(&b->players);
	free(b);
}

static void base_init(struct base *base)
{
	memset(base, 0, sizeof(*base));
	INIT_LIST_HEAD(&base->items);
	pthread_rwlock_init(&base->items_lock, NULL);
	ptrlist_init(&base->players);
}

#define BASE_CARGO_RANDOMNESS 0.5
static int base_genesis(struct base *base, struct planet *planet)
{
	unsigned int i, j;

	base->planet = planet;
	base->type = ptrlist_random(&planet->type->base_types);
	base->docks = 1; /* FIXME */

	struct base_type_item *bt_item;
	struct base_item *item;
	struct list_head *lh;
	list_for_each_entry(bt_item, &base->type->items, list) {
		item = malloc(sizeof*(item));
		if (!item)
			goto err;
		base_item_init(item);

		item->item = bt_item->item;
		item->max = bt_item->capacity * (1 - BASE_CARGO_RANDOMNESS)
			+ mtrandom_ulong(bt_item->capacity * BASE_CARGO_RANDOMNESS * 2);
		item->daily_change = bt_item->daily_change * (1 - BASE_CARGO_RANDOMNESS)
			+ mtrandom_long(bt_item->daily_change * BASE_CARGO_RANDOMNESS * 2);

		item->amount = mtrandom_ulong(item->max);
		if (item->amount > 10)
			item->amount = pow(5, log10(item->amount));

		list_add(&item->list, &base->items);
	}

	/* FIXME: limit loop */
	do {
		free(base->name);
		base->name = create_unique_name(&univ.avail_base_names);
	} while (st_lookup_exact(&univ.basenames, base->name));

	return 0;

err:
	base_free(base);
	return -1;
}

void base_populate_planet(struct planet* planet)
{
	struct base *b;
	int num;

	if (ptrlist_len(&planet->type->base_types) > 0)
		num = 10; /* FIXME */
	else
		num = 0;

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
