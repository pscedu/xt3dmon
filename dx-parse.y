/* $Id$ */

%{

#include "mon.h"

#include <ctype.h>
#include <err.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
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
%token DGT_CORKSCREW
%token DGT_CUBAN8
%token DGT_CURLYQ
%token DGT_CYCLENC
%token DGT_DMODE
%token DGT_EXIT
%token DGT_HL
%token DGT_MOVE
%token DGT_NODESYNC
%token DGT_OPT
%token DGT_ORBIT
%token DGT_PANEL
%token DGT_PIPEMODE
%token DGT_PLAYREEL
%token DGT_PSTICK
%token DGT_REFOCUS
%token DGT_REFRESH
%token DGT_SELJOB
%token DGT_SELNODE
%token DGT_SETCAP
%token DGT_STALL
%token DGT_SUBSEL
%token DGT_VMODE
%token DGT_WINSP
%token DGT_WIOFF

%token COMMA LANGLE LBRACKET RANGLE RBRACKET LS_MINUS LS_PLUS

%token <string>	STRING
%token <intg>	INTG
%token <dbl>	DBL

%type <dbl>	dbl;
%type <dbl>	orbit_revs orbit_secs;
%type <dbl>	move_secs;
%type <string>	cycle_method;
%type <intg>	dim;
%type <intg>	setmodifier;
%type <intg>	opts opts_l panels panels_l;

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
				if (strcasecmp(options[j].opt_abbr, $1) == 0) {
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

cycle_method	: {
			if (($$ = strdup("cycle")) == NULL)
				yyerror("strdup");
		}
		| STRING		{ $$ = $1; }
		;

orbit_revs	:			{ $$ = 1.0; }
		| dbl			{ $$ = $1; }
		;

orbit_secs	:			{ $$ = 0.0; }
		| INTG 			{ $$ = $1; }
		;

move_secs	:			{ $$ = 3.0; }
		| dbl			{ $$ = $1; }
		;

dim		: STRING {
			if (strcasecmp($1, "x") == 0)
				$$ = DIM_X;
			else if (strcasecmp($1, "y") == 0)
				$$ = DIM_Y;
			else if (strcasecmp($1, "z") == 0)
				$$ = DIM_Z;
			else
				yyerror("invalid dimension: %s", $1);
			free($1);
		}
		;

conf		: DGT_BIRD {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_BIRD;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);
		}
		| DGT_SETCAP STRING  {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SETCAP;
			dxa.dxa_caption = $2;
			dxa_add(&dxa);
		}
		| DGT_CLRSN {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CLRSN;
			dxa_add(&dxa);
		}
		| DGT_CORKSCREW dim {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CORKSCREW;
			dxa.dxa_screw_dim = $2;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);
		}
		| DGT_CUBAN8 dim {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CUBAN8;
			dxa.dxa_cuban8_dim = $2;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);
		}
		| DGT_CURLYQ setmodifier dim orbit_revs orbit_secs {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CURLYQ;
			dxa.dxa_orbit_dir = ($2 == DXV_OFF) ? -1 : 1;
			dxa.dxa_orbit_dim = $3;

			/* Avoid any FPE. */
			if ($4 == 0.0)
				yyerror("invalid curlyq #revs: %f", $4);
			dxa.dxa_orbit_frac = fabs($4);

			if ($5 < 0)
				yyerror("invalid curlyq #secs: %d", $5);
			dxa.dxa_orbit_secs = $5;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);
		}
		| DGT_CYCLENC cycle_method {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CYCLENC;
			if (strcmp($2, "grow") == 0)
				dxa.dxa_cycle_meth = DACM_GROW;
			else if (strcmp($2, "cycle") == 0)
				dxa.dxa_cycle_meth = DACM_CYCLE;
			else
				yyerror("invalid cycle method: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_DMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_DMODE;
			if (strcasecmp($2, "job") == 0)
				dxa.dxa_dmode = DM_JOB;
			else if (strcasecmp($2, "rte") == 0)
				dxa.dxa_dmode = DM_RTUNK;
			else if (strcasecmp($2, "yod") == 0)
				dxa.dxa_dmode = DM_YOD;
			else if (strcasecmp($2, "temp") == 0 ||
			    strcasecmp($2, "tmp") == 0 ||
			    strcasecmp($2, "temperature") == 0)
				dxa.dxa_dmode = DM_TEMP;
			else
				yyerror("invalid dmode: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_EXIT {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_EXIT;
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
		}
		| DGT_MOVE STRING dbl move_secs {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_MOVE;
			if (strcasecmp($2, "forward") == 0 ||
			    strcasecmp($2, "forw") == 0)
				dxa.dxa_move_dir = DIR_FORW;
			else if (strcasecmp($2, "back") == 0 ||
			    strcasecmp($2, "backward") == 0)
				dxa.dxa_move_dir = DIR_BACK;
			else
				yyerror("invalid move direction: %s", $2);
			free($2);
			dxa.dxa_move_amt = $3;
			if ($4 <= 0)
				yyerror("invalid move #secs: %f", $4);
			dxa.dxa_move_secs = $4;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);
		}
		| DGT_OPT setmodifier opts_l {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_OPT;
			dxa.dxa_opt_mode = $2;
			dxa.dxa_opts = $3;
			dxa_add(&dxa);
		}
		| DGT_ORBIT setmodifier dim orbit_revs orbit_secs {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_ORBIT;
			dxa.dxa_orbit_dir = ($2 == DXV_OFF) ? -1 : 1;
			dxa.dxa_orbit_dim = $3;

			/* Avoid any FPE. */
			if ($4 == 0.0)
				yyerror("invalid orbit #revs: %f", $4);
			dxa.dxa_orbit_frac = fabs($4);

			if ($5 < 0)
				yyerror("invalid orbit #secs: %d", $5);
			dxa.dxa_orbit_secs = $5;
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_CAMSYNC;
			dxa_add(&dxa);
		}
		| DGT_PANEL setmodifier panels_l {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_PANEL;
			dxa.dxa_panel_mode = $2;
			dxa.dxa_panels = $3;
			dxa_add(&dxa);
		}
		| DGT_PIPEMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_PIPEMODE;
			if (strcasecmp($2, "torus") == 0)
				dxa.dxa_pipemode = PM_DIR;
			else if (strcasecmp($2, "rte") == 0)
				dxa.dxa_pipemode = PM_RTE;
			else
				yyerror("invalid pipemode: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_PSTICK STRING panels_l {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			if (strcasecmp($2, "tl") == 0)
				dxa.dxa_pstick = PSTICK_TL;
			else if (strcasecmp($2, "tr") == 0)
				dxa.dxa_pstick = PSTICK_TR;
			else if (strcasecmp($2, "bl") == 0)
				dxa.dxa_pstick = PSTICK_BL;
			else if (strcasecmp($2, "br") == 0)
				dxa.dxa_pstick = PSTICK_BR;
			else
				yyerror("invalid pstick: %s", $2);
			free($2);
			dxa.dxa_type = DGT_PSTICK;
			dxa.dxa_panels = $3;
			dxa_add(&dxa);
		}
		| DGT_PLAYREEL INTG STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_PLAYREEL;
			dxa.dxa_reel_delay = $2;
			dxa.dxa_reel = $3;
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
		| DGT_STALL dbl {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa.dxa_stall_secs = $2;
			dxa_add(&dxa);
		}
		| DGT_SUBSEL STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SUBSEL;
			if (strcasecmp($2, "visible") == 0)
				dxa.dxa_selnode = DXSN_VIS;
			else
				yyerror("invalid subselection: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_VMODE STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_VMODE;
			if (strcasecmp($2, "phys") == 0)
				dxa.dxa_vmode = VM_PHYS;
			else if (strcasecmp($2, "wired") == 0)
				dxa.dxa_vmode = VM_WIRED;
			else if (strcasecmp($2, "wione") == 0)
				dxa.dxa_vmode = VM_WIONE;
			else
				yyerror("invalid vmode: %s", $2);
			free($2);
			dxa_add(&dxa);

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_NODESYNC;
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
		}
		| STRING args_l {
			yyerror("unrecognized action: %s", $1);
			free($1);
		}
		;

args_l		: STRING {
			free($1);
		}
		| args_l STRING {
			free($2);
		}
		;

%%

int
yyerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: %s:%d: ", progname, dx_fn, lineno);
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
