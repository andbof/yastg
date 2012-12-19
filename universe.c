#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "common.h"
#include "log.h"
#include "universe.h"
#include "item.h"
#include "list.h"
#include "rbtree.h"
#include "ptrlist.h"
#include "planet.h"
#include "base.h"
#include "system.h"
#include "civ.h"
#include "star.h"
#include "constellation.h"
#include "stringtree.h"
#include "mtrandom.h"

/*
 * The universe consists of a number of systems. These systems are grouped in constellations,
 * with the constellations having limited access between one another.
 *
 * Example: (. is a system, -|\/ are valid travel routes)
 * 
 * .-.-.     .-.-.
 *  \  |    /  | |
 *   .-.---.---.-.
 *   |    /
 *   .-.-.
 *   | | |
 *   .-.-.
 *
 */

#define NEIGHBOUR_CHANCE 5		/* The higher the value, the more neighbours a system will have */
#define NEIGHBOUR_DISTANCE_LY (25 * TICK_PER_LY)

struct universe univ;

void universe_free(struct universe *u)
{
	ptrlist_free(&u->systems);

	struct item *i, *_i;
	list_for_each_entry_safe(i, _i, &u->items, list) {
		list_del(&i->list);
		item_free(i);
		free(i);
	}

	st_destroy(&u->systemnames, ST_DONT_FREE_DATA);
	st_destroy(&u->planetnames, ST_DONT_FREE_DATA);
	st_destroy(&u->basenames, ST_DONT_FREE_DATA);
	if (u->name)
		free(u->name);
}

void linksystems(struct system *s1, struct system *s2)
{
	ptrlist_push(&s1->links, s2);
	ptrlist_push(&s2->links, s1);
}

int makeneighbours(struct system *s1, struct system *s2, unsigned long min, unsigned long max)
{
	unsigned long x, y;

	do {
		if (max > min) {
			x = min + mtrandom_ulong(max - min) + s1->x;
			y = min + mtrandom_ulong(max - min) + s1->y;
		} else {
			x = mtrandom_ulong(NEIGHBOUR_DISTANCE_LY) * 2 - NEIGHBOUR_DISTANCE_LY + s1->x;
			y = mtrandom_ulong(NEIGHBOUR_DISTANCE_LY) * 2 - NEIGHBOUR_DISTANCE_LY + s1->y;
		}

	} while (system_move(s2, x, y));

	return 0;
	/*
	 * FIXME: Don't place too close to another system.
	 * Perhaps try a limited number of times, then return failure.
	 */
}

static struct system* get_first_system_after_x(const struct rb_root * const root, const long x)
{
	struct system *system;
	struct rb_node *parent = NULL;
	struct rb_node *node;

	node = root->rb_node;
	while (node) {
		parent = node;
		system = rb_entry(node, struct system, x_rbtree);

		if (x < system->x)
			node = node->rb_left;
		else if (x > system->x)
			node = node->rb_right;
		else
			return rb_entry(node, struct system, x_rbtree);
	}

	return rb_entry(parent, struct system, x_rbtree);
}

unsigned long get_neighbouring_systems(struct ptrlist * const neighbours,
		const struct system * const origin, const long max_distance)
{
	struct system *system;
	long min_x, max_x;
	long min_y, max_y;
	struct rb_node *node;
	unsigned long neighbour_count = 0;

	min_x = origin->x - max_distance;
	max_x = origin->x + max_distance;
	min_y = origin->y - max_distance;
	max_y = origin->y + max_distance;
	system = get_first_system_after_x(&univ.x_rbtree, min_x);
	node = &system->x_rbtree;

	/*
	 * The extra y-coordinate comparison before the call to
	 * system_distance() improves performance quite considerably when
	 * measured (with perf and gcc -O3) as system_distance() is quite slow.
	 */
	while (node && rb_entry(node, struct system, x_rbtree)->x <= max_x) {

		system = rb_entry(node, struct system, x_rbtree);

		if (system->y >= min_y && system->y <= max_y &&
			(system_distance(origin, system) < max_distance)) {

			neighbour_count++;
			if (neighbours)
				ptrlist_push(neighbours, system);
		}

		node = rb_next(node);
	}

	return neighbour_count;
}

static struct system* get_system_at_x(const long x)
{
	struct rb_node *node = univ.x_rbtree.rb_node;
	struct system *system;

	while (node) {
		system = rb_entry(node, struct system, x_rbtree);

		if (x < system->x)
			node = node->rb_left;
		else if (x > system->x)
			node = node->rb_right;
		else
			return rb_entry(node, struct system, x_rbtree);
	}

	return NULL;
}

static void insert_system_into_rbtree(struct system * const s)
{
	struct rb_node **link = &univ.x_rbtree.rb_node;
	struct rb_node *parent = NULL;
	struct system *system;

	while (*link) {
		parent = *link;
		system = rb_entry(parent, struct system, x_rbtree);

		if (s->x < system->x)
			link = &(*link)->rb_left;
		else if (s->x > system->x)
			link = &(*link)->rb_right;
		else
			bug("System %s already inserted at %ldx%ld",
					system->name, system->x, system->y);
	}

	rb_link_node(&s->x_rbtree, parent, link);
	rb_insert_color(&s->x_rbtree, &univ.x_rbtree);
}

int system_move(struct system * const s, const long x, const long y)
{
	struct system *prev;

	/*
	 * All systems need to have unique x coordinates or there will be tree
	 * collisions. This is the most sane place to do that check.
	 */
	if ((prev = get_system_at_x(x)))
		return 1;

	/*
	 * This function is also used to set a position for freshly created
	 * systems which don't exist in the tree yet.
	 */
	prev = get_system_at_x(s->x);
	if (prev)
		rb_erase(&prev->x_rbtree, &univ.x_rbtree);

	s->x = x;
	s->y = y;

	insert_system_into_rbtree(s);

	return 0;
}

void universe_init(struct universe *u)
{
	time(&u->created);
	u->id = 0;
	u->name = NULL;
	ptrlist_init(&u->systems);
	INIT_LIST_HEAD(&u->items);
	INIT_LIST_HEAD(&u->systemnames);
	pthread_rwlock_init(&u->systemnames_lock, NULL);
	INIT_LIST_HEAD(&u->planetnames);
	pthread_rwlock_init(&u->planetnames_lock, NULL);
	INIT_LIST_HEAD(&u->basenames);
	pthread_rwlock_init(&u->basenames_lock, NULL);
}

int universe_genesis(struct universe *univ, struct civ *civs)
{
	/*
	 * 1. Decide number of constellations in universe.
	 * 2. For each constellation, create a number of systems, grouping them together.
	 */
	if (loadconstellations(univ))
		return -1;

	/*
	 * 3. Randomly distribute civilizations
	 * 4. Let civilizations grow and create hyperspace links
	 */
	civ_spawncivs(univ, civs);

	return 0;
}
