#ifndef _HAS_PARSECONFIG_RENAME
#define _HAS_PARSECONFIG_RENAME

/*
 * This file is created as per the recommendation of the automake
 * documentation, specifically section 8.8 "Yacc and Lex support".
 *
 * yylval has been removed and yyg has been added to support the reentrant parser.
 *
 * https://www.gnu.org/software/automake/manual/html_node/Yacc-and-Lex.html
 */

#define yymaxdepth	config_maxdepth
#define yyparse		config_parse
#define yylex		config_lex
#define yyerror		config_error
#define yychar		config_char
#define yydebug		config_debug
#define yypact		config_pact
#define yyr1		config_r1
#define yyr2		config_r2
#define yydef		config_def
#define yychk		config_chk
#define yypgo		config_pgo
#define yyact		config_act
#define yyexca		config_exca
#define yyerrflag	config_errflag
#define yynerrs		config_nerrs
#define yyps		config_ps
#define yypv		config_pv
#define yys		config_s
#define yy_yys		config_yys
#define yystate		config_state
#define yytmp		config_tmp
#define yyv		config_v
#define yy_yyv		config_yyv
#define yyval		config_val
#define yylloc		config_lloc
#define yyreds		config_reds
#define yytoks		config_toks
#define yylhs		config_yylhs
#define yylen		config_yylen
#define yydefred	config_yydefred
#define yydgoto 	config_yydgoto
#define yysindex	config_yysindex
#define yyrindex	config_yyrindex
#define yygindex	config_yygindex
#define yytable		config_yytable
#define yycheck		config_yycheck
#define yyname		config_yyname
#define yyrule		config_yyrule
#define yyg		config_yyg

#endif
