#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "port.h"
#include "cargo.h"
#include "common.h"
#include "stringtrie.h"
#include "item.h"
#include "log.h"
#include "mtrandom.h"
#include "planet.h"
#include "planet_type.h"
#include "universe.h"

void port_free(struct port *b)
{
	if (b->name) {
		st_rm_string(&univ.portnames, b->name);
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

static void port_init(struct port *port)
{
	memset(port, 0, sizeof(*port));
	INIT_LIST_HEAD(&port->items);
	pthread_rwlock_init(&port->items_lock, NULL);
	st_init(&port->item_names);
	ptrlist_init(&port->players);
}

#define PORT_CARGO_RANDOMNESS 0.5
static int port_genesis(struct port *port, struct planet *planet)
{
	port->planet = planet;
	port->system = planet->system;
	port->type = ptrlist_random(&planet->type->port_types);

	struct cargo *port_cargo, *cargo, *req;
	struct list_head *lh;
	list_for_each_entry(port_cargo, &port->type->items, list) {
		cargo = malloc(sizeof(*cargo));
		if (!cargo)
			goto err;
		cargo_init(cargo);

		cargo->item = port_cargo->item;
		cargo->max = port_cargo->max * (1 - PORT_CARGO_RANDOMNESS)
			+ mtrandom_ulong(port_cargo->max * PORT_CARGO_RANDOMNESS * 2);
		cargo->daily_change = port_cargo->daily_change * (1 - PORT_CARGO_RANDOMNESS)
			+ mtrandom_long(port_cargo->daily_change * PORT_CARGO_RANDOMNESS * 2);
		cargo->price = port_cargo->item->base_price;

		cargo->amount = mtrandom_ulong(cargo->max);
		if (cargo->amount > 10)
			cargo->amount = pow(5, log10(cargo->amount));

		if (st_add_string(&port->item_names, cargo->item->name, cargo))
			goto err;

		list_add(&cargo->list, &port->items);
	}

	/*
	 * We can't copy the requirement lists before all the port items are constructed
	 * and registered in the string tree or we wouldn't be able to look them up.
	 */
	list_for_each_entry(port_cargo, &port->type->items, list) {
		cargo = st_lookup_string(&port->item_names, port_cargo->item->name);
		ptrlist_for_each_entry(req, &port_cargo->requires, lh)
			ptrlist_push(&cargo->requires, st_lookup_string(&port->item_names, req->item->name));
	}

	port->name = create_unique_name(&univ.avail_port_names);

	list_add(&port->list, &univ.ports);

	return 0;

err:
	port_free(port);
	return -1;
}

#define PORT_MAXNUM 3
#define PORT_MUL_ODDS 2
void port_populate_planet(struct planet* planet)
{
	struct port *b;
	int num;

	num = 0;

	if (ptrlist_len(&planet->type->port_types) > 0) {
		while (num < PORT_MAXNUM && mtrandom_uint(UINT_MAX) < UINT_MAX / PORT_MUL_ODDS)
			num++;
	} else {
		num = 0;
	}

	pthread_rwlock_wrlock(&univ.portnames_lock);

	for (int i = 0; i < num; i++) {
		b = malloc(sizeof(*b));
		if (!b)
			goto unlock;
		port_init(b);
		if (port_genesis(b, planet)) {
			free(b);
			goto unlock;
		}
		ptrlist_push(&planet->ports, b);
		st_add_string(&univ.portnames, b->name, b);
	}

unlock:
	pthread_rwlock_unlock(&univ.portnames_lock);
}
