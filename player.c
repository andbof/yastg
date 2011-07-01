#include <stdlib.h>
#include "player.h"

void player_free(void *ptr) {
  struct player *pl = (struct player*)ptr;
  free(pl->name);
  free(pl);
}
