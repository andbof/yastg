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
#include "list.h"
#include "rbtree.h"
#include "ptrlist.h"
#include "planet.h"
#include "base.h"
#include "sector.h"
#include "civ.h"
#include "star.h"
#include "constellation.h"
#include "stringtree.h"
#include "mtrandom.h"

/*
 * The universe consists of a number of sectors. These sectors are grouped in constellations,
 * with the constellations having limited access between one another.
 *
 * Example: (. is a sector, -|\/ are valid travel routes)
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
#define NEIGHBOUR_DISTANCE_LY (50 * TICK_PER_LY)

struct universe univ;

void universe_free(struct universe *u)
{
	ptrlist_free(&u->sectors);
	st_destroy(&u->sectornames, ST_DONT_FREE_DATA);
	st_destroy(&u->planetnames, ST_DONT_FREE_DATA);
	st_destroy(&u->basenames, ST_DONT_FREE_DATA);
	if (u->name)
		free(u->name);
}

void linksectors(struct sector *s1, struct sector *s2)
{
	ptrlist_push(&s1->links, s2);
	ptrlist_push(&s2->links, s1);
}

int makeneighbours(struct sector *s1, struct sector *s2, unsigned long min, unsigned long max)
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

	} while (sector_move(s2, x, y));

	return 0;
	/*
	 * FIXME: Don't place too close to another system.
	 * Perhaps try a limited number of times, then return failure.
	 */
}

static struct sector* get_first_sector_after_x(const struct rb_root * const root, const long x)
{
	struct sector *sector;
	struct rb_node *parent = NULL;
	struct rb_node *node;

	node = root->rb_node;
	while (node) {
		parent = node;
		sector = rb_entry(node, struct sector, x_rbtree);

		if (x < sector->x)
			node = node->rb_left;
		else if (x > sector->x)
			node = node->rb_right;
		else
			return rb_entry(node, struct sector, x_rbtree);
	}

	return rb_entry(parent, struct sector, x_rbtree);
}

struct ptrlist* get_neighbouring_systems(struct ptrlist * const neighbours,
		const struct sector * const origin, const long max_distance)
{
	struct sector *sector;
	long min_x, max_x;
	long min_y, max_y;
	struct rb_node *node;

	min_x = origin->x - max_distance;
	max_x = origin->x + max_distance;
	min_y = origin->y - max_distance;
	max_y = origin->y + max_distance;
	sector = get_first_sector_after_x(&univ.x_rbtree, min_x);
	node = &sector->x_rbtree;

	/*
	 * The extra y-coordinate comparison before the call to
	 * sector_distance() is an optimization as we have to traverse a _lot_
	 * of systems and sector_distance() is quite slow. Hopefully it does
	 * what is intended.
	 */
	while (node && rb_entry(node, struct sector, x_rbtree)->x <= max_x) {

		sector = rb_entry(node, struct sector, x_rbtree);

		if (sector->y >= min_y && sector->y <= max_y &&
			(sector_distance(origin, sector) < max_distance))
			ptrlist_push(neighbours, sector);

		node = rb_next(node);
	}

	return neighbours;
}

size_t count_neighbouring_systems(const struct sector * const origin,
		const unsigned long max_distance)
{
	struct sector *sector;
	long min_x, max_x;
	long min_y, max_y;
	struct rb_node *node;
	size_t count = 0;

	min_x = origin->x - max_distance;
	max_x = origin->x + max_distance;
	min_y = origin->y - max_distance;
	max_y = origin->y + max_distance;
	sector = get_first_sector_after_x(&univ.x_rbtree, min_x);
	node = &sector->x_rbtree;

	while (node && rb_entry(node, struct sector, x_rbtree)->x <= max_x) {

		sector = rb_entry(node, struct sector, x_rbtree);

		if (sector->y >= min_y && sector->y <= max_y &&
			(sector_distance(origin, sector) < max_distance))
			count++;

		node = rb_next(node);
	}

	return count;
}

static struct sector* get_sector_at_x(const long x)
{
	struct rb_node *node = univ.x_rbtree.rb_node;
	struct sector *sector;

	while (node) {
		sector = rb_entry(node, struct sector, x_rbtree);

		if (x < sector->x)
			node = node->rb_left;
		else if (x > sector->x)
			node = node->rb_right;
		else
			return rb_entry(node, struct sector, x_rbtree);
	}

	return NULL;
}

static void insert_sector_into_rbtree(struct sector * const s)
{
	struct rb_node **link = &univ.x_rbtree.rb_node;
	struct rb_node *parent = NULL;
	struct sector *sector;

	while (*link) {
		parent = *link;
		sector = rb_entry(parent, struct sector, x_rbtree);

		if (s->x < sector->x)
			link = &(*link)->rb_left;
		else if (s->x > sector->x)
			link = &(*link)->rb_right;
		else
			bug("Sector %s already inserted at %ldx%ld",
					sector->name, sector->x, sector->y);
	}

	rb_link_node(&s->x_rbtree, parent, link);
	rb_insert_color(&s->x_rbtree, &univ.x_rbtree);
}

int sector_move(struct sector * const s, const long x, const long y)
{
	struct sector *prev;

	/*
	 * All sectors need to have unique x coordinates or there will be tree
	 * collisions. This is the most sane place to do that check.
	 */
	if ((prev = get_sector_at_x(x)))
		return 1;

	/*
	 * This function is also used to set a position for freshly created
	 * sectors which don't exist in the tree yet.
	 */
	prev = get_sector_at_x(s->x);
	if (prev)
		rb_erase(&prev->x_rbtree, &univ.x_rbtree);

	s->x = x;
	s->y = y;

	insert_sector_into_rbtree(s);

	return 0;
}

void universe_init(struct universe *u)
{
	time(&u->created);
	u->id = 0;
	u->name = NULL;
	u->numsector = 0;
	ptrlist_init(&u->sectors);
	INIT_LIST_HEAD(&u->sectornames);
	pthread_rwlock_init(&u->sectornames_lock, NULL);
	INIT_LIST_HEAD(&u->planetnames);
	pthread_rwlock_init(&u->planetnames_lock, NULL);
	INIT_LIST_HEAD(&u->basenames);
	pthread_rwlock_init(&u->basenames_lock, NULL);
}

int universe_genesis(struct universe *univ, struct civ *civs)
{
	int i;
	int power = 0;
	struct civ *c;

	list_for_each_entry(c, &civs->list, list)
		power += c->power;

	/*
	 * 1. Decide number of constellations in universe.
	 * 2. For each constellation, create a number of sectors, grouping them together.
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
