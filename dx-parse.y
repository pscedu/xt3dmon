/* $Id$ */

%{

#include "mon.h"

#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <string.h>

#include "deusex.h"
#include "env.h"
#include "panel.h"
#include "state.h"

#define yyin dxin

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);

extern int lineno;
int errors;
double dbl;
struct dxlist dxlist = TAILQ_HEAD_INITIALIZER(dxlist);

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
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_BIRD;
			dxa_add(&dxa);
		}
		| DAT_CAPTION STRING  {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_CAPTION;
			dxa.dxa_caption = $2;
			dxa_add(&dxa);
		}
		| DAT_CLRSN {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_CLRSN;
			dxa_add(&dxa);
		}
		| DAT_CYCLENC {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_CYCLENC;
			dxa_add(&dxa);
		}
		| DAT_DMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_DMODE;
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
		| DAT_HL STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_HL;
			if (strcasecmp($2, "all") == 0)
				dxa.dxa_hl = HL_ALL;
			else if (strcasecmp($2, "seldm") == 0)
				dxa.dxa_hl = HL_SELDM;
			else
				yyerror("invalid hl: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DAT_MOVE STRING l_dbl {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_MOVE;
			if (strcasecmp($2, "forward") == 0 ||
			    strcasecmp($2, "forw") == 0)
				dxa.dxa_move_dir = DIR_FORW;
			else if (strcasecmp($2, "back") == 0)
				dxa.dxa_move_dir = DIR_BACK;
			else
				yyerror("invalid move direction: %s", $2);
			free($2);
			dxa.dxa_move_amt = dbl;
			dxa_add(&dxa);
		}
		| DAT_OPT STRING {
			struct dx_action dxa;
			char *p, *t;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_OPT;
			p = $2;
			switch (*p) {
			case '-':
				dxa.dxa_opt_mode = DXV_OFF;
				break;
			case '+':
				dxa.dxa_opt_mode = DXV_ON;
				break;
			default:
				dxa.dxa_opt_mode = DXV_SET;
				break;
			}
			for (; p != NULL; p = t) {
				if ((t = strchr(p, ',')) != NULL)
					*t++ = '\0';
				while (isspace(*p))
					p++;
				if (strcasecmp(p, "nlabels"))
					dxa.dxa_opts |= OP_NLABELS;
				else if (strcasecmp(p, "pipes"))
					dxa.dxa_opts |= OP_PIPES;
				else if (strcasecmp(p, "skel"))
					dxa.dxa_opts |= OP_SKEL;
				else if (strcasecmp(p, "frames"))
					dxa.dxa_opts |= OP_FRAMES;
				else if (strcasecmp(p, "tween"))
					dxa.dxa_opts |= OP_TWEEN;
				else if (strcasecmp(p, "display"))
					dxa.dxa_opts |= OP_DISPLAY;
				else if (strcasecmp(p, "nodeanim"))
					dxa.dxa_opts |= OP_NODEANIM;
				else if (strcasecmp(p, "caption"))
					dxa.dxa_opts |= OP_CAPTION;
				else if (strcasecmp(p, "deusex"))
					dxa.dxa_opts |= OP_DEUSEX;
				else
					yyerror("invalid option: %s", p);
			}
			if (dxa.dxa_opts == 0)
				yyerror("no options specified");
			free($2);
			dxa_add(&dxa);
		}
		| DAT_ORBIT STRING {
			struct dx_action dxa;
			char *p;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_ORBIT;
			p = $2;
			if (p[0] == '-') {
				p++;
				dxa.dxa_orbit_dir = -1;
			} else
				dxa.dxa_orbit_dir = 1;
			if (strcasecmp(p, "x"))
				dxa.dxa_orbit_dim = DIM_X;
			else if (strcasecmp(p, "y"))
				dxa.dxa_orbit_dim = DIM_Y;
			else if (strcasecmp(p, "z"))
				dxa.dxa_orbit_dim = DIM_Z;
			else
				yyerror("invalid orbit dimension: %s", p);
			free($2);
			dxa_add(&dxa);
		}
		| DAT_PANEL STRING {
			struct dx_action dxa;
			char *p, *t;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_PANEL;
			p = $2;
			switch (*p) {
			case '-':
				p++;
				dxa.dxa_panel_mode = DXV_OFF;
				break;
			case '+':
				p++;
				dxa.dxa_panel_mode = DXV_ON;
				break;
			default:
				dxa.dxa_panel_mode = DXV_SET;
				break;
			}
			for (; p != NULL; p = t) {
				if ((t = strchr(p, ',')) != NULL)
					*t++ = '\0';
				while (isspace(*p))
					p++;
				if (strcasecmp(p, "compass"))
					dxa.dxa_panels |= PANEL_CMP;
				else if (strcasecmp(p, "wiadj"))
					dxa.dxa_panels |= PANEL_WIADJ;
				else if (strcasecmp(p, "legend"))
					dxa.dxa_panels |= PANEL_LEGEND;
				else if (strcasecmp(p, "help"))
					dxa.dxa_panels |= PANEL_HELP;
				else
					yyerror("invalid panel: %s", p);
			}
			if (dxa.dxa_panels == 0)
				yyerror("no panels specified");
			free($2);
			dxa_add(&dxa);
		}
		| DAT_REFOCUS {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_REFOCUS;
			dxa_add(&dxa);
		}
		| DAT_REFRESH {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_REFRESH;
			dxa_add(&dxa);
		}
		| DAT_SELJOB STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_SELJOB;
			if (strcasecmp($2, "random") == 0)
				dxa.dxa_seljob = DXSJ_RND;
			else
				yyerror("invalid job: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DAT_SELNODE INTG {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_SELNODE;
			dxa.dxa_selnode = $2;
			dxa_add(&dxa);
		}
		| DAT_SELNODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_SELNODE;
			if (strcasecmp($2, "random") == 0)
				dxa.dxa_selnode = DXSN_RND;
			else
				yyerror("invalid node: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DAT_VMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_VMODE;
			if (strcasecmp($2, "phys") == 0)
				dxa.dxa_vmode = VM_PHYS;
			else if (strcasecmp($2, "wione") == 0)
				dxa.dxa_vmode = VM_WIONE;
			else
				yyerror("invalid vmode: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DAT_WINSP {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_WINSP;
			dxa_add(&dxa);
		}
		| DAT_WIOFF {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DAT_WIOFF;
			dxa_add(&dxa);
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

	dxa_clear();
	if ((fp = fopen(fn, "r")) == NULL)
		err(1, "%s", fn);
	yyin = fp;
	lineno = 1;
	yyparse();
	fclose(fp);

	if (errors)
		errx(1, "%d error(s) encountered", errors);
}
