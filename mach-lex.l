/* $Id$ */

%{

#include "mon.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "phys-parse.h"

#define yyerror physerror
#define yylval physlval

int yyerror(const char *, ...);

extern int lineno;

%}

%%

contains			{ return (CONTAINS); }
dim				{ return (DIM); }
mag				{ return (MAG); }
offset				{ return (OFFSET); }
size				{ return (SIZE); }
space				{ return (SPACE); }
spans				{ return (SPANS); }
skel				{ return (SKEL); }

"{"				{ return (LBRACKET); }
"}"				{ return (RBRACKET); }
"<"				{ return (LANGLE); }
">"				{ return (RANGLE); }
,				{ return (COMMA); }

[0-9]+				{
					yylval.intg = atoi(yytext);
					return (INTG);
				}
-?[0-9]+\.[0-9]*|-?\.[0-9]+	{
					errno = 0;
					yylval.dbl = strtod(yytext, NULL);
					if (errno)
						yyerror("%s", strerror(errno));
					return (DBL);
				}
[a-zA-Z_]+			{
					if ((yylval.string = strdup(yytext)) == NULL)
						yyerror("%s", strerror(errno));
					return (STRING);
				}
#.*				{ ; }
\n				{ lineno++; }
[\t ]+				{ ; }
.				{ yyerror("%s: invalid token", yytext); }

%%

int
yywrap(void)
{
	return (1);
}
