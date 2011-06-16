#include <string.h>
#include <math.h>
#include "universe.h"
#include "civ.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "defines.h"
#include "array.h"
#include "sarray.h"
#include "parseconfig.h"
#include "id.h"
#include "star.h"

struct sector* initsector() {
  struct sector *s;
  MALLOC_DIE(s, sizeof(*s));
  memset(s, 0, sizeof(*s));
  s->id = gen_id();
  s->phi = 0.0;
  s->linkids = sarray_init(sizeof(size_t), 0, SARRAY_ENFORCE_UNIQUE, NULL, &sort_id);
  return s;
}

void sector_free(void *ptr) {
  struct sector *s = ptr;
  if (s->name)
    free(s->name);
  if (s->gname)
    free(s->gname);
  sarray_free(s->stars);
  free(s->stars);
  sarray_free(s->planets);
  free(s->planets);
  sarray_free(s->bases);
  free(s->bases);
  sarray_free(s->linkids);
  free(s->linkids);
}

struct sector* loadsector(struct configtree *ctree) {
  struct sector *s = initsector();
  struct planet *p;
  struct base *b;
  struct star *sol;
  size_t i;
  int haspos = 0;
  s->stars = sarray_init(sizeof(struct star), 0, SARRAY_ENFORCE_UNIQUE, &star_free, &sort_id);
  s->planets = sarray_init(sizeof(struct planet), 0, SARRAY_ENFORCE_UNIQUE, &planet_free, &sort_id);
  s->bases = sarray_init(sizeof(struct base), 0, SARRAY_ENFORCE_UNIQUE, &base_free, &sort_id);
  while (ctree) {
    if (strcmp(ctree->key, "GNAME") == 0) {
      s->gname = strdup(ctree->data);
      s->name = NULL;
    } else if (strcmp(ctree->key, "PLANET") == 0) {
      p = loadplanet(ctree->sub);
      sarray_add(s->planets, p);
      free(p);
    } else if (strcmp(ctree->key, "BASE") == 0) {
      b = loadbase(ctree->sub);
      sarray_add(s->bases, b);
      free(b);
    } else if (strcmp(ctree->key, "STAR") == 0) {
      sol = loadstar(ctree->sub);
      sarray_add(s->stars, sol);
      free(sol);
    } else if (strcmp(ctree->key, "POS") == 0) {
      sscanf(ctree->data, "%lu %lu", &s->x, &s->y);
      haspos = 1;
    }
    ctree = ctree->next;
  }
  for (i = 0; i < s->stars->elements; i++)
    s->hab += ((struct star*)sarray_getbypos(s->stars, i))->hab;
  if (!s->gname)
    die("%s", "required attribute missing in predefined sector: gname");
  if (!haspos)
    die("required attribute missing in predefined sector %s: position", s->name);
  s->hablow = ((struct star*)s->stars->array)->hablow;
  s->habhigh = ((struct star*)s->stars->array)->habhigh;
  s->snowline = ((struct star*)s->stars->array)->snowline;
  return s;
}

struct sector* createsector(char *name) {
  int i;
  struct star *sol;
  struct sector *s = initsector();
  s->name = strdup(name);
  s->stars = createstars();
  s->hab = 0;
  for (i = 0; i < s->stars->elements; i++) {
    sol = (struct star*)sarray_getbypos(s->stars, i);
    s->hab += sol->hab;
    MALLOC_DIE(sol->name, strlen(s->name)+3);
    sprintf(sol->name, "%s %c", s->name, i+65);
  }
  s->hab -= STELLAR_MUL_HAB*(i-1);
  s->hablow = ((struct star*)s->stars->array)->hablow;
  s->habhigh = ((struct star*)s->stars->array)->habhigh;
  s->snowline = ((struct star*)s->stars->array)->snowline;
  s->planets = createplanets(s);
  // FIXME: bases
  return s;
}

void sector_move(struct universe *u, struct sector *s, long x, long y) {
  struct ulong_id uid;
  struct double_id did;
  struct ptr_num *pn;
  struct sector *stmp;
  size_t st;
  s->x = x;
  s->y = y;
  uid.id = s->id;
  uid.i = XYTORAD(s->x, s->y);
  did.id = s->id;
  did.i = XYTOPHI(s->x, s->y);
  // We need to make sure we're not adding this sector twice to srad and sphi
  // However, they're not sorted by id so this is a bit cumbersome
  pn = sarray_getlnbyid(u->srad, &uid.i);
  for (st = 0; st < pn->num; st++) {
    stmp = (struct sector*)(pn->ptr+st*u->srad->element_size);
    if (stmp->id == s->id) {
      sarray_rmbyptr(u->srad, pn->ptr+st*u->srad->element_size);
      break;
    }
  }
  free(pn);
  pn = sarray_getlnbyid(u->sphi, &did.i);
  for (st = 0; st < pn->num; st++) {
    stmp = (struct sector*)(pn->ptr+st*u->sphi->element_size);
    if (stmp->id == s->id) {
      sarray_rmbyptr(u->sphi, pn->ptr+st*u->sphi->element_size);
      break;
    }
  }
  free(pn);
  // Now update the coordinates and add them to srad and sphi
  s->r = uid.i;
  sarray_add(u->srad, &uid);
  s->phi = did.i;
  sarray_add(u->sphi, &did);
//  printf("Moved sector %zx (%s) to %ldx%ld (%lux%1.6f)\n", s->id, s->name, s->x, s->y, s->r, s->phi);
}

unsigned long getdistance(struct sector *a, struct sector *b) {
  long result = sqrt( (double)(b->x-a->x)*(b->x-a->x) + (double)(b->y-a->y)*(b->y-a->y) );
  if (result < 0)
    return -result;
  return result;
}
