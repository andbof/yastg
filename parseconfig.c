#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "defines.h"
#include "log.h"
#include "parseconfig.h"
#include "id.h"

/*
 * Duplicates a string but removes whitespace from the beginning and end of the string.
 * If the string consists of just whitespace, an empty string is returned.
 */
char* strdupsp(char *s) {
  long unsigned int i = 0;
  char *t;
  i = 0;
  while ((i < strlen(s)) && ((s[i] == ' ') || (s[i] == '\t'))) i++;
  if (i < strlen(s)) {
    t = strdup(s+i);
    i = strlen(t);
    while ((t[i] == ' ') || (t[i] == '\t')) i--;
    t[i] = '\0';
  } else {
    t = NULL;
  }
  return t;
} 

/*
 * Parses config files with the following syntax
 *
 * KEY1 DATA
 * KEY2 DATA
 * KEY4 DATA {
 *   KEY4.1 DATA
 *   KEY4.2 DATA
 * }
 *
 * and returns a configtree structure.
 */
struct configtree* parseconfig(char *fname) {
  FILE *f;
  struct configtree *root;
  MALLOC_DIE(root, sizeof(*root));
  if (!(f = fopen(fname, "r"))) {
    die("%s", "parseconfig failed to open file\n");
  }
  root->key = strdup(fname);
  root->data = NULL;
  root->next = NULL;
  root->sub = recparseconfig(f, fname);
  printf("Done with %s", fname);
  fclose(f);
  return root;
}

struct configtree* recparseconfig(FILE *f, char *fname) {
  struct configtree *root = NULL, *ce = NULL;
  int hassub;
  size_t i, j;
  unsigned long linen = 1;
  size_t buflen = 128, bufidx = 0;
  // FIXME: vad händer med bufidx här? ska inte den uppdateras?
  char *buf;
  MALLOC_DIE(buf, buflen);
  char *c;
  while (fgets(buf+bufidx, buflen-bufidx, f) != NULL) {
    if (!(strchr(buf, '\n'))) {
      // We haven't read a whole line. Increase the buffer size
      // and try again.
      if (buflen < INT_MAX << 2) {
	buflen <<= 2;
      } else if (buflen == INT_MAX) {
	die("line %lu too long in %s", linen, fname);
      } else {
	buflen = INT_MAX;
      }
      REALLOC_DIE(buf, buflen);
    } else {
      // Remove line break
      c = strchr(buf, '\n');
      *c = '\0';

      // Remove comments
      c = strchr(buf, '#');
      if (c)
	*c = '\0';

      if (strlen(buf) > 0) {

	// Remove preceding whitespace
	i = 0;
	while ((buf[i] == ' ') || (buf[i] == '\t'))
	  i++;

	// This is the end of a sub
	if (buf[i] == '}')
	  break;

	// Allocate space for field
	if (root == NULL) {
	  MALLOC_DIE(root, sizeof(*root));
	  ce = root;
	} else {
	  MALLOC_DIE(ce->next, sizeof(*(ce->next)));
	  ce = ce->next;
	  ce->next = NULL;
	}

	// Extract key
	j = i;
	while ((j < strlen(buf)) && (buf[j] != ' '))
	  j++;
	if (j == strlen(buf)) {
          ce->key = strdupsp(buf+i);
	  ce->data = NULL;
	  ce->sub = NULL;
//	  printf("ce->key is %s, ce->data is NULL\n", ce->key);
	} else {
	  buf[j] = '\0';
	  ce->key = strdupsp(buf+i);
	  j++;

	  // Determine if this line is a parent or not (trailed by '{')
	  hassub = 0;
	  if ((c = strchr(buf+j, '{'))) {
	    *c = '\0';
	    hassub = 1;
	  }

	  // Remove trailing whitespace
//	  printf("j is %d\n", j);
	  c = strchr(buf+j, '\0')-1;
	  while ((*c == ' ' || *c == '\t' ) && (*c != '\0'))
	    c--;
//	  printf("c is \"%c\"\n", *c);
	  if (c == '\0') {
	    ce->data = NULL;
	  } else {
	    *(c+1) = '\0';
	    ce->data = strdupsp(buf+j);
	  }

	  // If this has subelements, parse them recursively
	  if (hassub) {
	    ce->sub = recparseconfig(f, fname);
	  } else {
	    ce->sub = NULL;
	  }

	}

      }
    }
  }

  free(buf);
  return root;
}

/*
 * Recursively free all subnodes in a configtree.
 * Will free the entire tree if called on root node.
 */
void destroyctree(struct configtree *ctree) {
  struct configtree *ptree;
  while (ctree) {
    destroyctree(ctree->sub);
    if (ctree->key) free(ctree->key);
    if (ctree->data) free(ctree->data);
    ptree = ctree->next;
    free(ctree);
    ctree = ptree;
  }
}
