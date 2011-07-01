#include <stdlib.h>
#include "player.h"

void player_free(void *ptr) {
  free(ptr);
}
