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
#include "list.h"
#include "parseconfig-yacc.h"

#define YY_EXTRA_TYPE struct list_head*

extern int yyparse();
%}

%%

[ \t]		;
[#][^\n]*	;
[\n]		return EOL;
[{]		return LEFTCURLY;
[}]		return RIGHTCURLY;
[^{}" \t\n]+	{
			yylval->str = strdup(yytext);
			return WORD;
		}
["][^"\n]*["]	{
			yylval->str = strdup(yytext + 1);
			yylval->str[strlen(yylval->str) -1] = '\0';
			return WORD;
		}
%%

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
