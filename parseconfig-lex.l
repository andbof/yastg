%option noyywrap
%option 8bit
%option reentrant
%option warn nodefault
%option yylineno
%option bison-bridge
%option never-interactive

%option noinput
%option nounput

%{

#include "parseconfig-rename.h"
#include <stdlib.h>
#include <limits.h>
#include "list.h"
#include "parseconfig-yacc.h"

#define YY_EXTRA_TYPE struct list_head*

extern int yyparse();
static long parse_scientific(const char * const number);

%}

%%

[ \t]		;
[#][^\n]*	;
[\n]		return EOL;
[{]		return LEFTCURLY;
[}]		return RIGHTCURLY;

0x[0-9a-fA-F]+		{
				yylval->l = strtol(yytext, NULL, 16);
				return NUMBER;
			}

-?[0-9]+[eE][0-9]+	{
				yylval->l = parse_scientific(yytext);
				return NUMBER;
			}

-?[0-9]+		{
				yylval->l = strtol(yytext, NULL, 10);
				return NUMBER;
			}

[a-zA-Z][a-zA-Z0-9\.]*	{
				yylval->str = strdup(yytext);
				return WORD;
			}

["][^"\n]*["]		{
				yylval->str = strdup(yytext + 1);
				yylval->str[strlen(yylval->str) -1] = '\0';
				return WORD;
			}

%%

static long raise_ten_to_power(long exp)
{
	if (exp == 0)
		return 1;

	long r = 1;
	for (long l = 0; l < exp; l++) {
		r *= 10;
		if (r > LONG_MAX / 10)
			return LONG_MAX;
	}

	return r;
}

static long parse_scientific(const char * const string)
{
	const char *pexp = string;
	while (*pexp != 'e' && *pexp != 'E')
		pexp++;
	pexp++;

	long exp = strtol(pexp, NULL, 10);
	long coeff = strtol(string, NULL, 10);

	exp = raise_ten_to_power(exp);
	if (LONG_MAX / exp < coeff)
		return LONG_MAX;
	else if (LONG_MIN / exp > coeff)
		return LONG_MIN;
	else
		return exp * coeff;
}

int parse(char *begin, size_t size, struct list_head * const root)
{
	void *scanner;
	if (yylex_init_extra(root, &scanner))
		return -1;

	YY_BUFFER_STATE yybs = yy_scan_buffer(begin, size, scanner);
	if (!yybs)
		goto err_destroy;
	yy_switch_to_buffer(yybs, scanner);

	if (yyparse(scanner))
		goto err_delete;

	yy_delete_buffer(yybs, scanner);
	yylex_destroy(scanner);

	return 0;

err_delete:
	yy_delete_buffer(yybs, scanner);
err_destroy:
	yylex_destroy(scanner);

	return -1;
}
