%{
#define YYDEBUG 1

#include "parseconfig-rename.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <ctype.h>
#include <unistd.h>
#include "list.h"
#include "log.h"
#include "parseconfig.h"
#include "parseconfig-yacc.h"

extern int yylex(YYSTYPE *yylval, void *yyscanner);
extern int parse(char *begin, size_t size, struct list_head * const root);
extern struct list_head* yyget_extra(void *yyscanner);
extern void yyset_extra(struct list_head *state, void *yyscanner);
extern int yyget_lineno(void *yyscanner);

struct state {
	struct list_head list;
	struct list_head *root;
};

static void yyerror(void *yyscanner, const char * const msg)
{
	log_printfn("config", "parse error on line %d: %s", yyget_lineno(yyscanner) + 1, msg);
}

static void init_config(struct config * const config)
{
	memset(config, 0, sizeof(*config));
	INIT_LIST_HEAD(&config->children);
	INIT_LIST_HEAD(&config->list);
}

static struct config* insert_new_element(struct list_head *root)
{
	struct state *s = list_first_entry(root, struct state, list);
	struct config *c = malloc(sizeof(*c));
	if (!c)
		return NULL;

	init_config(c);
	list_add_tail(&c->list, s->root);

	return c;
}

static int push_state(struct list_head *children, void *yyscanner)
{
	struct list_head *root = yyget_extra(yyscanner);
	struct state *new_s = malloc(sizeof(*new_s));
	if (!new_s)
		return -1;

	new_s->root = children;
	INIT_LIST_HEAD(&new_s->list);
	list_add(&new_s->list, root);

	return 0;
}

static int pop_state(void *yyscanner)
{
	struct state *s = list_first_entry(yyget_extra(yyscanner), struct state, list);
	if (list_len(&s->list) < 2)
		return -1;

	list_del(&s->list);
	free(s);

	return 0;
}
%}

%pure-parser
%lex-param {void *yyscanner}
%parse-param {void *yyscanner}
%error-verbose

%union {
	char *str;
	long l;
}

%token	EOL
%token	NUMBER
%token	WORD
%left 	LEFTCURLY
%left 	RIGHTCURLY

%type	<l>	NUMBER
%type	<str>	WORD

%%

input:	/* empty */
     |	input line
     ;

line:	EOL
    |	WORD EOL {
		struct config *c = insert_new_element(yyget_extra(yyscanner));
		if (!c) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
		c->key = $1;
	}
    |	WORD WORD EOL {
		struct config *c = insert_new_element(yyget_extra(yyscanner));
		if (!c) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
		c->key = $1;
		c->str = $2;
	}
    |	WORD NUMBER EOL {
		struct config *c = insert_new_element(yyget_extra(yyscanner));
		if (!c) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
		c->key = $1;
		c->l = $2;
	}
    |	WORD NUMBER WORD EOL {
		struct config *c = insert_new_element(yyget_extra(yyscanner));
		if (!c) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
		c->key = $1;
		c->l = $2;
		c->str = $3;
	}
    |	WORD LEFTCURLY EOL {
		struct config *c = insert_new_element(yyget_extra(yyscanner));
		if (!c) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
		c->key = $1;
		if (push_state(&c->children, yyscanner)) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
	}
    |	WORD WORD LEFTCURLY EOL {
		struct config *c = insert_new_element(yyget_extra(yyscanner));
		if (!c) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
		c->key = $1;
		c->str = $2;
		if (push_state(&c->children, yyscanner)) {
			yyerror(yyscanner, "out of memory");
			YYABORT;
		}
	}
    |	RIGHTCURLY EOL {
		if (pop_state(yyscanner)) {
			yyerror(yyscanner, "superfluous '}'");
			YYERROR;
		}
	}
    ;

%%

static int has_proper_terminators(char * const begin, const off_t size)
{
	off_t i;
	for (i = 0; i < size - 2; i++)
		if (begin[i] == '\0')
			return 0;

	if (begin[size - 2] != '\0')
		return 0;
	if (begin[size - 1] != '\0')
		return 0;

	return 1;
}

int parse_config_mmap(char *begin, const off_t size, struct list_head * const root)
{
	struct list_head state_root;
	struct state state;

	INIT_LIST_HEAD(&state_root);
	state.root = root;
	list_add(&state.list, &state_root);

	if (!has_proper_terminators(begin, size)) {
		log_printfn("config", "file contains NULLs and cannot be parsed");
		return -1;
	}

	if (parse(begin, size, &state_root))
		return -1;

	return 0;
}

int parse_config_file(const char * const fname, struct list_head * const root)
{
	int fd;
	struct stat s;

	if (!(fd = open(fname, O_RDONLY)))
		return -1;

	if (flock(fd, LOCK_SH) != 0)
		goto err_close;

	if (fstat(fd, &s))
		goto err_close;

	char *begin;
	if ((begin = mmap(NULL, s.st_size + 2, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_POPULATE, fd, 0)) == MAP_FAILED)
		goto err_close;

	begin[s.st_size] = '\0';
	begin[s.st_size + 1] = '\0';

	log_printfn("config", "parsing \"%s\"", fname);
	if (parse_config_mmap(begin, s.st_size + 2, root))
		goto err_unmap;

	log_printfn("config", "successfully parsed \"%s\"", fname);

	munmap(begin, s.st_size);
	close(fd);

	return 0;

err_unmap:
	munmap(begin, s.st_size);
err_close:
	close(fd);

	return -1;
}

void destroy_config(struct list_head * const root)
{
	struct config *c, *_c;
	list_for_each_entry_safe(c, _c, root, list) {
		destroy_config(&c->children);
		list_del(&c->list);
		free(c->key);
		if (c->str)
			free(c->str);
		free(c);
	}
}
