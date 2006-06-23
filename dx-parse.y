/* $Id$ */

%{

#include "mon.h"

#include <err.h>
#include <stdarg.h>
#include <string.h>

#include "deusex.h"
#include "state.h"

#define yyin dxin

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);

extern int lineno;
int errors;
double dbl;
struct dx_action dxa;

%}

%token DAT_BIRD
%token DAT_CAPTION
%token DAT_CLRSN
%token DAT_CYCLENC
%token DAT_DMODE
%token DAT_HL
%token DAT_MOVE
%token DAT_OPT
%token DAT_ORBIT
%token DAT_PANEL
%token DAT_REFOCUS
%token DAT_REFRESH
%token DAT_SELJOB
%token DAT_SELNODE
%token DAT_VMODE
%token DAT_WINSP
%token DAT_WIOFF

%token COMMA LANGLE LBRACKET RANGLE RBRACKET

%token <string> STRING
%token <intg> INTG
%token <strict_dbl> STRICT_DBL

%union {
	char	*string;
	int	 intg;
	double	 strict_dbl;
};

%%

grammar		: /* empty */
		| grammar conf {
		}
		;

l_dbl		: STRICT_DBL {
			dbl = $1;
		}
		| INTG {
			dbl = $1;
		}
		;

conf		: DAT_BIRD {
		}
		| DAT_CAPTION STRING  {
		}
		| DAT_CLRSN {
		}
		| DAT_CYCLENC {
		}
		| DAT_DMODE STRING {
			memset(&dxa, 0, sizeof(dxa));
			if (strcasecmp($2, "job") == 0)
				dxa.dxa_dmode = DM_JOB;
			else if (strcasecmp($2, "temp") == 0 ||
			    strcasecmp($2, "tmp") == 0 ||
			    strcasecmp($2, "temperature") == 0)
				dxa.dxa_dmode = DM_TEMP;
			else
				yyerror("invalid dmode: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DAT_HL {
		}
		| DAT_MOVE {
		}
		| DAT_OPT {
		}
		| DAT_ORBIT {
		}
		| DAT_PANEL {
		}
		| DAT_REFOCUS {
		}
		| DAT_REFRESH {
		}
		| DAT_SELJOB {
		}
		| DAT_SELNODE {
		}
		| DAT_VMODE {
		}
		| DAT_WINSP {
		}
		| DAT_WIOFF {
		}
		;

%%

int
yyerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", progname, lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);

	errors++;
	return (0);
}

void
dx_parse(const char *fn)
{
	FILE *fp;
	extern FILE *yyin;

	if ((fp = fopen(fn, "r")) == NULL)
		err(1, "%s", fn);
	yyin = fp;
	lineno = 1;
	yyparse();
	fclose(fp);

	if (errors)
		errx(1, "%d error(s) encountered", errors);
}
