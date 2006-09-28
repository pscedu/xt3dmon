/* $Id$ */

%{

#include "mon.h"

#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <string.h>

#include "deusex.h"
#include "env.h"
#include "nodeclass.h"
#include "panel.h"
#include "pathnames.h"
#include "state.h"

#define yyin dxin

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);

extern int lineno;
int errors;
struct dxlist dxlist = TAILQ_HEAD_INITIALIZER(dxlist);

%}

%token DGT_BIRD
%token DGT_CAMSYNC
%token DGT_CLRSN
%token DGT_CYCLENC
%token DGT_DMODE
%token DGT_HL
%token DGT_MOVE
%token DGT_NODESYNC
%token DGT_OPT
%token DGT_ORBIT
%token DGT_PANEL
%token DGT_PSTICK
%token DGT_REFOCUS
%token DGT_REFRESH
%token DGT_SELJOB
%token DGT_SELNODE
%token DGT_SETCAP
%token DGT_STALL
%token DGT_VMODE
%token DGT_WINSP
%token DGT_WIOFF

%token COMMA LANGLE LBRACKET RANGLE RBRACKET LS_MINUS LS_PLUS

%token <string> STRING
%token <intg> INTG
%token <dbl> DBL

%type <dbl> dbl;
%type <intg> setmodifier;
%type <intg> opts opts_l panels panels_l;

%union {
	char	*string;
	int	 intg;
	double	 dbl;
};

%%

grammar		: /* empty */
		| grammar conf {
		}
		;

dbl		: INTG			{ $$ = $1; }
		| DBL			{ $$ = $1; }
		;

setmodifier	: /* empty */		{ $$ = DXV_SET; }
		| LS_PLUS		{ $$ = DXV_ON;  }
		| LS_MINUS 		{ $$ = DXV_OFF; }
		;

opts		: STRING {
			int j;

			for (j = 0; j < NOPS; j++)
				if (strcasecmp(opts[j].opt_abbr, $1) == 0) {
					$$ = 1 << j;
					break;
				}
			if (j == NOPS)
				yyerror("unknown option: %s", $1);
			free($1);
		}
		;

opts_l		: opts			{ $$ = $1; }
		| opts COMMA opts_l	{ $$ = $1 | $3; }
		;

panels		: STRING {
			int j;

			for (j = 0; j < NPANELS; j++)
				if (strcasecmp(pinfo[j].pi_abbr, $1) == 0) {
					$$ = 1 << j;
					break;
				}
			if (j == NPANELS)
				yyerror("unknown panel: %s", $1);
			free($1);
		}
		;

panels_l	: panels		{ $$ = $1; }
		| panels COMMA panels_l	{ $$ = $1 | $3; }
		;

conf		: DGT_BIRD {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_BIRD;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_SETCAP STRING  {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SETCAP;
			dxa.dxa_caption = $2;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_CLRSN {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CLRSN;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_CYCLENC {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CYCLENC;
			dxa_add(&dxa);
		}
		| DGT_DMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_DMODE;
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
		| DGT_HL STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_HL;
			if (strcasecmp($2, "all") == 0)
				dxa.dxa_hl = NC_ALL;
			else if (strcasecmp($2, "seldm") == 0)
				dxa.dxa_hl = NC_SELDM;
			else
				yyerror("invalid hl: %s", $2);
			free($2);
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_MOVE STRING dbl {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_MOVE;
			if (strcasecmp($2, "forward") == 0 ||
			    strcasecmp($2, "forw") == 0)
				dxa.dxa_move_dir = DIR_FORW;
			else if (strcasecmp($2, "back") == 0)
				dxa.dxa_move_dir = DIR_BACK;
			else
				yyerror("invalid move direction: %s", $2);
			free($2);
			dxa.dxa_move_amt = $3;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_OPT setmodifier opts_l {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_OPT;
			dxa.dxa_opt_mode = $2;
			dxa.dxa_opts = $3;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_ORBIT setmodifier STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_ORBIT;
			dxa.dxa_orbit_dir = ($2 == DXV_OFF) ? -1 : 1;
			if (strcasecmp($3, "x") == 0)
				dxa.dxa_orbit_dim = DIM_X;
			else if (strcasecmp($3, "y") == 0)
				dxa.dxa_orbit_dim = DIM_Y;
			else if (strcasecmp($3, "z") == 0)
				dxa.dxa_orbit_dim = DIM_Z;
			else
				yyerror("invalid orbit dimension: %s", $3);
			free($3);
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_PANEL setmodifier panels_l {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_PANEL;
			dxa.dxa_panel_mode = $2;
			dxa.dxa_panels = $3;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_PSTICK STRING panels_l {
			struct dx_action dxa;
			int pstick;

			pstick = 0;
			if (strcasecmp($2, "tl"))
				pstick = PSTICK_TL;
			else if (strcasecmp($2, "tr"))
				pstick = PSTICK_TR;
			else if (strcasecmp($2, "bl"))
				pstick = PSTICK_BL;
			else if (strcasecmp($2, "br"))
				pstick = PSTICK_BR;
			else
				yyerror("invalid pstick: %s", $2);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_PSTICK;
			dxa.dxa_pstick = pstick;
			dxa.dxa_panels = $3;
			dxa_add(&dxa);
			free($2);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_REFOCUS {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_REFOCUS;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_REFRESH {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_REFRESH;
			dxa_add(&dxa);
		}
		| DGT_SELJOB STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SELJOB;
			if (strcasecmp($2, "random") == 0)
				dxa.dxa_seljob = DXSJ_RND;
			else
				yyerror("invalid job: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_SELNODE INTG {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SELNODE;
			dxa.dxa_selnode = $2;
			dxa_add(&dxa);
		}
		| DGT_SELNODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SELNODE;
			if (strcasecmp($2, "random") == 0)
				dxa.dxa_selnode = DXSN_RND;
			else
				yyerror("invalid node: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_VMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_VMODE;
			if (strcasecmp($2, "phys") == 0)
				dxa.dxa_vmode = VM_PHYS;
			else if (strcasecmp($2, "wione") == 0)
				dxa.dxa_vmode = VM_WIONE;
			else
				yyerror("invalid vmode: %s", $2);
			free($2);
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_NODESYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_WINSP setmodifier INTG setmodifier INTG setmodifier INTG {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_WINSP;
			dxa.dxa_winsp_mode.iv_x = $2;
			dxa.dxa_winsp_mode.iv_y = $4;
			dxa.dxa_winsp_mode.iv_z = $6;
			dxa.dxa_winsp.iv_x = $3;
			dxa.dxa_winsp.iv_y = $5;
			dxa.dxa_winsp.iv_z = $7;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_NODESYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa_add(&dxa);
		}
		| DGT_WIOFF setmodifier INTG setmodifier INTG setmodifier INTG {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_WIOFF;
			dxa.dxa_wioff_mode.iv_x = $2;
			dxa.dxa_wioff_mode.iv_y = $4;
			dxa.dxa_wioff_mode.iv_z = $6;
			dxa.dxa_wioff.iv_x = $3;
			dxa.dxa_wioff.iv_y = $5;
			dxa.dxa_wioff.iv_z = $7;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_NODESYNC;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
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
dx_parse(void)
{
	extern FILE *yyin;
	FILE *fp;

	dxa_clear();
	if ((fp = fopen(dx_fn, "r")) == NULL)
		err(1, "%s", dx_fn);
	yyin = fp;
	lineno = 1;
	yyparse();
	fclose(fp);

	if (errors)
		errx(1, "%s: %d error(s) encountered", dx_fn, errors);

	dx_built = 1;
}
