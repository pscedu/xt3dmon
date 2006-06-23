/* $Id$ */

%{

#include "mon.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "dx-parse.h"

#define yyerror dxerror
#define yylval dxlval

int yyerror(const char *, ...);

extern int lineno;

%}

%%

bird					{ return (DAT_BIRD); }
caption					{ return (DAT_CAPTION); }

clearselnode				{ return (DAT_CLRSN); }
clearselnodes				{ return (DAT_CLRSN); }
clearsn					{ return (DAT_CLRSN); }
clearsns				{ return (DAT_CLRSN); }
clrselnode				{ return (DAT_CLRSN); }
clrselnodes				{ return (DAT_CLRSN); }
clrsn					{ return (DAT_CLRSN); }
clrsns					{ return (DAT_CLRSN); }

cyclenc					{ return (DAT_CYCLENC); }

dmode					{ return (DAT_DMODE); }
dm					{ return (DAT_DMODE); }

hl					{ return (DAT_HL); }
move					{ return (DAT_MOVE); }

options					{ return (DAT_OPT); }
option					{ return (DAT_OPT); }
opt					{ return (DAT_OPT); }
op					{ return (DAT_OPT); }

orbit					{ return (DAT_ORBIT); }

panels					{ return (DAT_PANEL); }
panel					{ return (DAT_PANEL); }

refocus					{ return (DAT_REFOCUS); }
refresh					{ return (DAT_REFRESH); }

seljob					{ return (DAT_SELJOB); }
sj					{ return (DAT_SELJOB); }

selnode					{ return (DAT_SELNODE); }
sn					{ return (DAT_SELNODE); }

vmode					{ return (DAT_VMODE); }
vm					{ return (DAT_VMODE); }

winsp					{ return (DAT_WINSP); }
wioff					{ return (DAT_WIOFF); }

"<"					{ return (LANGLE); }
">"					{ return (RANGLE); }
,					{ return (COMMA); }
-					{ return (LS_MINUS); }
\+					{ return (LS_PLUS); }

[0-9]+					{
						yylval.intg = atoi(yytext);
						return (INTG);
					}
-?[0-9]+\.[0-9]*|-?\.[0-9]+		{
						errno = 0;
						yylval.dbl = strtod(yytext, NULL);
						if (errno)
							yyerror("%s", strerror(errno));
						return (DBL);
					}
[a-zA-Z_]+				{
						if ((yylval.string = strdup(yytext)) == NULL)
							yyerror("%s", strerror(errno));
						return (STRING);
					}
\"[^"]*\"				{
						char *endp;

						if ((endp = strrchr(yytext, '"')) == NULL)
							yyerror("lexer missed quote");
						endp = '\0';
						if ((yylval.string = strdup(yytext + 1)) == NULL)
							yyerror("%s", strerror(errno));
						return (STRING);
					}
#.*					{ ; }
\n					{ lineno++; }
[\t ]+					{ ; }
.					{ yyerror("%s: invalid token", yytext); }

%%

int
yywrap(void)
{
	return (1);
}
