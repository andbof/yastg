#ifndef _HAS_STRINGTREE_H
#define _HAS_STRINGTREE_H

/*
 * The trie will look somewhat like this for the strings "tert" and "test",
 * pointing to 0xbeef and 0xface, respectively.
 * Dashed boxes represent one st_root / st_lsb struct. Only initialized array
 * data is shown, if something is omitted assume it is 0 or NULL.
 *
 * One character is stored in two data structures: "root" and "lsb".
 * "root" contains the most significant three bits while "lsb" are the least
 * significant three bits of the six bit total. Bits 6 and 7 in the byte can
 * _never_ be set, i.e. only the values 0--63 can be stored.
 *
 * To simplify the structure, the lsb for the last character always points to
 * a new st_root that points to the data itself (or to other lsbs if the
 * string looked up is a substring of another string).
 *
 * +-st_root_1-----------+
 * | msb[6] => st_lsb_1  |  int(T) - 32 = 52, msb(52) = 0b110 = 6
 * +---------------------+
 *
 * +-st_lsb_1------------+
 * | lsb[4] => st_root_2 |  int(T) - 32 = 52, lsb(52) = 0b100 = 4
 * +---------------------+
 *
 * +-st_root_2-----------+
 * | msb[4] => st_lsb_2  |  int(E) - 32 = 37, msb(37) = 0b100 = 6
 * +---------------------+
 *
 * +-st_lsb_2------------+
 * | lsb[5] => st_root_3 |  int(E) - 32 = 37, lsb(37) = 0b101 = 5
 * +---------------------+
 *
 * +-st_root_3-----------+
 * | msb[6] => st_lsb_4  |  int(R) - 32 = 50, msb(50) = 0b110 = 6
 * +---------------------+  int(S) - 32 = 51, msb(51) = 0b110 = 6
 *
 * +-st_lsb_4------------+
 * | lsb[2] => st_root_5 |  int(R) - 32 = 50, lsb(50) = 0b010 = 2
 * | lsb[3] => st_root_6 |  int(S) - 32 = 51, lsb(51) = 0b011 = 3
 * +---------------------+
 *
 * +-st_root_5-----------+
 * | msb[6] => st_lsb_5  |  int(T) - 32 = 52, msb(52) = 0b110 = 6
 * +---------------------+
 *
 * +-st_root_6-----------+
 * | msb[6] => st_lsb_6  |  int(T) - 32 = 52, msb(52) = 0b110 = 6
 * +---------------------+
 *
 * +-st_lsb_5------------+
 * | lsb[6] => st_root_7 | int(T) - 32 = 52, lsb(52) = 0b100 = 4
 * +---------------------+
 *
 * +-st_lsb_6------------+
 * | lsb[6] => st_root_8 | int(T) - 32 = 52, lsb(52) = 0b100 = 4
 * +---------------------+
 *
 * +-st_root_7------+
 * | data => 0xbabe |  int(T) - 32 = 52, lsb(52) = 0b100 = 4
 * +----------------+
 *
 * +-st_root_8------+
 * | data => 0xface |  int(T) - 32 = 52, lsb(52) = 0b100 = 4
 * +----------------+
 *
 */

struct st_lsb;

struct st_root {
	struct st_lsb *msb[8];
	void *data;
};

enum st_free_data {
	ST_DONT_FREE_DATA,
	ST_DO_FREE_DATA
};

void st_init(struct st_root * const root);
void st_destroy(struct st_root * const root,
		const enum st_free_data do_free_data);

int st_add_string(struct st_root * const root, const char *string, void *data);

void *st_lookup_string(const struct st_root * const root,
		const char * const string);
void *st_lookup_exact(const struct st_root * const root,
		const char * const string);

void *st_rm_string(struct st_root * const root, const char * const string);
int st_is_empty(const struct st_root * const root);
void st_foreach_data(struct st_root *root,
		void (*func)(void *data, const char *string, void *hints),
		void *hints);

#endif
