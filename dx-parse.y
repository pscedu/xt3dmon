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
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "pathnames.h"
#include "seastar.h"
#include "state.h"
#include "status.h"

#define yyin dxin

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);

void nidlist_add(struct nidlist *, int);

int dx_lineno;
static int errors;
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
%token DGT_FOCUS
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
%token DGT_SELNC
%token DGT_SELNODE
%token DGT_SETCAP
%token DGT_SSCTL
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
%type <intg>	cycle_method;
%type <intg>	dim;
%type <intg>	setmodifier;
%type <intg>	opts opts_l panels panels_l;
%type <nidlist>	nidlist subsel_list selnode_list;

%union {
	char		*string;
	int		 intg;
	double		 dbl;
	struct nidlist	*nidlist;
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

cycle_method	:			{ $$ = DACM_CYCLE; }
		| STRING {
			if (strcasecmp($1, "grow") == 0)
				$$ = DACM_GROW;
			else if (strcasecmp($1, "cycle") == 0)
				$$ = DACM_CYCLE;
			else
				yyerror("invalid cycle method: %s", $1);
			free($1);
		}
		;

orbit_revs	:			{ $$ = 1.0; }
		| dbl			{ $$ = $1; }
		;

orbit_secs	:			{ $$ = 0.0; }
		| dbl 			{ $$ = $1; }
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

subsel_list	: nidlist		{ $$ = $1; }
		;

selnode_list	: nidlist		{ $$ = $1; }
		;

nidlist		: STRING {
			struct nidlist *nl;
			char *p, *t, *s;
			long l;

			if ((nl = malloc(sizeof(*nl))) == NULL)
				err(1, "malloc");
			SLIST_INIT(nl);

			if (strcasecmp($1, "selected") == 0)
				nidlist_add(nl, DXN_SEL);
			else if (strcasecmp($1, "all") == 0)
				nidlist_add(nl, DXN_ALL);
			else if (strcasecmp($1, "visible") == 0)
				nidlist_add(nl, DXN_VIS);
			else if (strcasecmp($1, "random") == 0)
				nidlist_add(nl, DXN_RND);
			else {
				for (p = $1; p != NULL; p = t) {
					if ((t = strchr(p, ',')) != NULL)
						*t++ = '\0';
					s = p;
					while (isdigit(*s))
						s++;
					if (*s == '\0' &&
					    (l = strtol(p, NULL, 10)) >= 0 &&
					    l < NID_MAX)
						nidlist_add(nl, l);
					else
						yyerror("invalid node ID: %s", p);
				}
			}
			free($1);
			$$ = nl;

			if (SLIST_EMPTY(nl)) {
				yyerror("no nodes specified");
				free(nl);
				$$ = NULL;
			} else
				$$ = nl;
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
			dxa.dxa_cycle_meth = $2;;
			dxa_add(&dxa);
		}
		| DGT_DMODE STRING {
			struct dx_action dxa;
			int i;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_DMODE;
			for (i = 0; i < NDM; i++)
				if (dmodes[i].dm_abbr &&
				    strcasecmp(dmodes[i].dm_abbr, $2) == 0) {
					dxa.dxa_dmode = i;
					break;
				}
			if (i >= NDM)
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
		| DGT_FOCUS dbl COMMA dbl COMMA dbl {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_FOCUS;
			dxa.dxa_focus.fv_x = $2;
			dxa.dxa_focus.fv_y = $4;
			dxa.dxa_focus.fv_z = $6;
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
		| DGT_PLAYREEL INTG STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_PLAYREEL;
			dxa.dxa_reel_delay = $2;
			dxa.dxa_reel = $3;
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
		| DGT_SELNC STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SELNC;
			if (strcasecmp($2, "random") == 0)
				dxa.dxa_selnc = DXNC_RND;
			else
				yyerror("invalid node class: %s", $2);
			free($2);
			dxa_add(&dxa);
		}
		| DGT_SELNODE setmodifier selnode_list {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SELNODE;
			dxa.dxa_selnode_mode = $2;
			dxa.dxa_selnode_list = $3;
			dxa_add(&dxa);
		}
		| DGT_SETCAP STRING  {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SETCAP;
			dxa.dxa_caption = $2;
			dxa_add(&dxa);
		}
		| DGT_STALL dbl {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_STALL;
			dxa.dxa_stall_secs = $2;
			dxa_add(&dxa);
		}
		| DGT_SSCTL STRING STRING {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SSCTL;
			if (strcasecmp($2, "vc") == 0) {
				dxa.dxa_ssctl_type = DXSST_VC;
				if (strcasecmp($3, "0") == 0)
					dxa.dxa_ssctl_value = 0;
				else if (strcasecmp($3, "1") == 0)
					dxa.dxa_ssctl_value = 1;
				else if (strcasecmp($3, "2") == 0)
					dxa.dxa_ssctl_value = 2;
				else if (strcasecmp($3, "3") == 0)
					dxa.dxa_ssctl_value = 3;
				else
					yyerror("invalid ssctl vc: %s", $3);
			} else if (strcasecmp($2, "mode") == 0) {
				dxa.dxa_ssctl_type = DXSST_MODE;

				if (strcasecmp($3, "cycles") == 0)
					dxa.dxa_ssctl_value = SSCNT_NBLK;
				else if (strcasecmp($3, "packets") == 0)
					dxa.dxa_ssctl_value = SSCNT_NPKT;
				else if (strcasecmp($3, "flits") == 0)
					dxa.dxa_ssctl_value = SSCNT_NFLT;
				else
					yyerror("invalid ssctl mode: %s", $3);
			} else
				yyerror("invalid ssctl type: %s", $2);
			free($2);
			free($3);
			dxa_add(&dxa);
		}
		| DGT_SUBSEL setmodifier subsel_list {
			struct dx_action dxa;

			memset(&dxa, 0, sizeof(dxa));
			dxa.dxa_type = DGT_SUBSEL;
			dxa.dxa_subsel_mode = $2;
			dxa.dxa_subsel_list = $3;
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

void
nidlist_add(struct nidlist *nl, int nid)
{
	struct nid *n;

	if ((n = malloc(sizeof(*n))) == NULL)
		err(1, "malloc");
	memset(n, 0, sizeof(*n));
	n->n_nid = nid;
	SLIST_INSERT_HEAD(nl, n, n_link);
}


int
yyerror(const char *fmt, ...)
{
	char errmsg[BUFSIZ];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);

	status_add(SLP_URGENT, "%s:%d: %s\n", dx_fn, dx_lineno, errmsg);

	errors++;
	return (0);
}

int
dx_parse(void)
{
	extern FILE *yyin;
	FILE *fp;
	int ret;

	dxa_clear();
	if ((fp = fopen(dx_fn, "r")) == NULL)
		err(1, "%s", dx_fn);
	yyin = fp;
	dx_lineno = 1;
	yyparse();
	fclose(fp);

	ret = (errors == 0);
	if (errors)
		status_add(SLP_URGENT, "%s: %d error(s)",
		    dx_fn, errors);
	errors = 0;
	return (ret);
}
