#ifndef HAS_PLAYER_H
#define HAS_PLAYER_H

struct player {
  char* name;
  size_t position;
};

void player_free();

static struct player *alfred;

#endif
