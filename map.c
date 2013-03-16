#include <alloca.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "asciiart.h"
#include "universe.h"
#include "map.h"
#include "system.h"

static char* flatten_array(char * out, size_t out_len,
		const size_t buf_size_x, const size_t buf_size_y,
		char buf[buf_size_y][buf_size_x])
{
	size_t row_len, y;

	for (y = 0; y < buf_size_y; y++) {
		row_len = strlen(buf[y]);
		if (out_len <= row_len)
			break;

		memcpy(out, buf[y], row_len);
		out += row_len;
		out_len -= row_len;

		if (y == buf_size_y - 1)
			break;

		*out = '\n';
		out++;
		out_len--;
	}
	*out = '\0';

	return out;
}

struct map_item {
	int x;
	int y;
	char *s;
	struct list_head list;
};

#define KEY_SPACING 2
static void plot_items_in_map(struct list_head *items,
		const size_t buf_size_x, const size_t buf_size_y,
		const size_t map_ul_x, const size_t map_ul_y,
		const size_t map_lr_x, const size_t map_lr_y,
		char buf[buf_size_y][buf_size_x])
{
	struct map_item *map_item;
	const size_t key_col = map_lr_x + KEY_SPACING;
	const size_t mid_x = (map_lr_x - map_ul_x) / 2 + map_ul_x;
	const size_t mid_y = (map_lr_y - map_ul_y) / 2 + map_ul_y;
	size_t row = 0;
	size_t ix, iy;
	char key;

	list_for_each_entry(map_item, items, list) {
		row++;

		if (row > buf_size_y)
			break;

		if (row == 1)
			key = 'X';
		else if (row < 10)
			key = row + '0';
		else if (row < 10 + 26)
			key = row - 10 + 'a';
		else if (row < 10 + 26 + 26)
			key = row - 10 + 'A';
		else
			break;

		ix = map_item->x + mid_x;
		ix = MIN(ix, map_lr_x - 1);
		ix = MAX(ix, map_ul_x + 1);
		iy = map_item->y + mid_y;
		iy = MIN(iy, map_lr_y - 1);
		iy = MAX(iy, map_ul_y + 1);

		if (buf[iy][ix] == ' ')
			buf[iy][ix] = key;
		else
			key = buf[iy][ix];

		for (ix = map_lr_x + 1; ix < key_col + 3; ix++)
			buf[row][ix] = ' ';

		buf[row][key_col] = key;
		memcpy(&buf[row][key_col + 2], map_item->s, strlen(map_item->s));
	}

	return;
}

char* generate_map(char * const out, const size_t len, struct system *origin,
		const unsigned long radius, const unsigned int width)
{
	const unsigned int x_size = width;
	const unsigned int y_size = x_size / ASCII_X_TO_Y;
	const unsigned int max_x = x_size + 29;
	const unsigned int max_y = y_size + 3;

	struct ptrlist neigh;
	struct list_head *lh;
	struct system *s;
	char buf[max_y][max_x];

	memset(buf, 0, sizeof(buf));

	/*
	 * By assuming a minimum size, we make the rest of this function less complicated,
	 * and beneath a certain width the map is not very useful anyway.
	 */
	if (width < 21)
		return NULL;

	if (draw_square(max_x, max_y, buf, 0, 0, width))
		return NULL;

	ptrlist_init(&neigh);
	get_neighbouring_systems(&neigh, origin, radius);
	ptrlist_push(&neigh, origin);

	ptrlist_sort(&neigh, origin, cmp_system_distances);

	szprintf(&buf[0][2], "%s", "SYSTEM MAP");
	szprintf(&buf[0][x_size / 2], "|<- %lu ly ", radius / TICK_PER_LY);
	buf[0][x_size - 2] = '>';

	const int tick_x = (radius * 2) / x_size;
	const int tick_y = (radius * 2) / y_size;
	struct map_item *map_item;
	LIST_HEAD(map_head);

	ptrlist_for_each_entry(s, &neigh, lh) {
		map_item = alloca(sizeof(*map_item));

		map_item->x = (s->x - origin->x)/tick_x;
		map_item->y = (s->y - origin->y)/tick_y;
		map_item->s = s->name;

		list_add_tail(&map_item->list, &map_head);
	}

	ptrlist_free(&neigh);

	plot_items_in_map(&map_head, max_x, max_y, 0, 0, x_size - 1, y_size - 1, buf);

	return flatten_array(out, len, max_x, max_y, buf);
}
