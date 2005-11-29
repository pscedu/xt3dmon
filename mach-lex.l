/* $Id$ */

%{

#include <errno.h>
#include <stdarg.h>

#include "mon.h"
#include "y.tab.h"

int yyerror(const char *, ...);

%}

%%

contains			{ return (CONTAINS); }
dim				{ return (DIM); }
mag				{ return (MAG); }
offset				{ return (OFFSET); }
size				{ return (SIZE); }
space				{ return (SPACE); }
spans				{ return (SPANS); }
{				{ return (LBRACKET); }
}				{ return (RBRACKET); }
"<"				{ return (LANGLE); }
">"				{ return (RANGLE); }
,				{ return (COMMA); }
[0-9]+				{
					yylval.wnumber = atoi(yytext);
					return (WNUMBER);
				}
[0-9]+\.[0-9]*|[0-9]*\.[0-9]+	{
					errno = 0;
					yylval.fnumber = strtod(yytext, NULL);
					if (errno)
						yyerror("%s", strerror(errno));
					return (FNUMBER);
				}
[a-zA-Z_]+			{
					if ((yylval.string = strdup(yytext)) == NULL)
						yyerror("%s", strerror(errno));
					return (STRING);
				}
#.*				{ ; }
[\n\t]				{ ; }
.				{ yyerror("invalid token"); }

%%

int
yywrap(void)
{
	return (1);
}
