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

bird					{ return (DGT_BIRD); }
clrsn					{ return (DGT_CLRSN); }
corkscrew				{ return (DGT_CORKSCREW); }
cuban8					{ return (DGT_CUBAN8); }
curlyq					{ return (DGT_CURLYQ); }
cyclenc					{ return (DGT_CYCLENC); }
dmode					{ return (DGT_DMODE); }
exit					{ return (DGT_EXIT); }
hl					{ return (DGT_HL); }
move					{ return (DGT_MOVE); }
opt					{ return (DGT_OPT); }
orbit					{ return (DGT_ORBIT); }
panel					{ return (DGT_PANEL); }
pipemode				{ return (DGT_PIPEMODE); }
playreel				{ return (DGT_PLAYREEL); }
pstick					{ return (DGT_PSTICK); }
refocus					{ return (DGT_REFOCUS); }
refresh					{ return (DGT_REFRESH); }
seljob					{ return (DGT_SELJOB); }
selnode					{ return (DGT_SELNODE); }
setcap					{ return (DGT_SETCAP); }
ssctl					{ return (DGT_SSCTL); }
stall					{ return (DGT_STALL); }
subsel					{ return (DGT_SUBSEL); }
vmode					{ return (DGT_VMODE); }
winsp					{ return (DGT_WINSP); }
wioff					{ return (DGT_WIOFF); }

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
						*endp = '\0';
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
