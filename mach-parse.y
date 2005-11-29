/* $Id$ */

%{

#include <err.h>
#include <stdarg.h>

#include "mon.h"

int yylex(void);
int yyerror(const char *, ...);

//static int lineno = 1;
static int errors;

%}

%token CONTAINS DIM MAG OFFSET SIZE SPACE SPANS
%token COMMA LANGLE LBRACKET RANGLE RBRACKET

%token <string> STRING
%token <wnumber> WNUMBER
%token <fnumber> FNUMBER

%type <fv> vector

%union {
	char		*string;
	int		 wnumber;
	double		 fnumber;
	struct fvec	 fv;
}

%%

grammar		: /* empty */
		| grammar '\n'
		| grammar conf '\n'
		| grammar error '\n'
		;

optnl		: '\n' optnl
		|
		;

conf		: DIM STRING '{' {
			free($2);
		}
		opts '}' {
		}
		;

opts		:
		;

%%

int
yyerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);

	errors++;
	return (0);
}
