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

#define INIT_DXA(type)				\
	do {					\
		memset(&dxa, 0, sizeof(dxa));	\
		dxa.dxa_type = (type);		\
		dxa_add(&dxa);			\
	} while (0)

int yylex(void);
int yyparse(void);
int yyerror(const char *, ...);

void nidlist_add(struct nidlist *, int);

int dx_lineno;
static int errors;
struct dxlist dxlist = TAILQ_HEAD_INITIALIZER(dxlist);
static struct dx_action dxa;

%}

%token <intg> DGT_BIRD
%token <intg> DGT_CAMSYNC		/* no grammar */
%token <intg> DGT_CAMLOOK
%token <intg> DGT_CAMPOS
%token <intg> DGT_CAMUPROT
%token <intg> DGT_CLRSN
%token <intg> DGT_CORKSCREW
%token <intg> DGT_CUBAN8
%token <intg> DGT_CURLYQ
%token <intg> DGT_CYCLENC
%token <intg> DGT_DMODE
%token <intg> DGT_EXIT
%token <intg> DGT_FOCUS
%token <intg> DGT_HL
%token <intg> DGT_MOVE
%token <intg> DGT_NODESYNC
%token <intg> DGT_OPT
%token <intg> DGT_ORBIT
%token <intg> DGT_PANEL
%token <intg> DGT_PIPEMODE
%token <intg> DGT_PLAYREEL
%token <intg> DGT_PSTICK
%token <intg> DGT_REFOCUS
%token <intg> DGT_REFRESH
%token <intg> DGT_SELNC
%token <intg> DGT_SELNODE
%token <intg> DGT_SETCAP
%token <intg> DGT_SSCTL
%token <intg> DGT_STALL
%token <intg> DGT_SUBSEL
%token <intg> DGT_VMODE
%token <intg> DGT_WINSP
%token <intg> DGT_WIOFF

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
			INIT_DXA($1);
			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_CAMLOOK dbl COMMA dbl COMMA dbl {
			INIT_DXA($1);
			vec_set(&dxa.dxa_cam_lv, $2, $4, $6);
			if (vec_mag(&dxa.dxa_cam_lv) == 0.0)
				vec_set(&dxa.dxa_cam_lv, 1.0, 0.0, 0.0);
			else
				vec_normalize(&dxa.dxa_cam_lv);
		}
		| DGT_CAMPOS dbl COMMA dbl COMMA dbl {
			INIT_DXA($1);
			vec_set(&dxa.dxa_cam_v, $2, $4, $6);
		}
		| DGT_CAMUPROT dbl {
			INIT_DXA($1);
			dxa.dxa_cam_ur = $2;
		}
		| DGT_CLRSN {
			INIT_DXA($1);
		}
		| DGT_CORKSCREW dim {
			INIT_DXA($1);
			dxa.dxa_screw_dim = $2;
			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_CUBAN8 dim {
			INIT_DXA($1);
			dxa.dxa_cuban8_dim = $2;
			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_CURLYQ setmodifier dim orbit_revs orbit_secs {
			INIT_DXA($1);
			dxa.dxa_orbit_dir = ($2 == DXV_OFF) ? -1 : 1;
			dxa.dxa_orbit_dim = $3;

			/* Avoid any FPE. */
			if ($4 == 0.0)
				yyerror("invalid curlyq #revs: %f", $4);
			dxa.dxa_orbit_frac = fabs($4);

			if ($5 < 0)
				yyerror("invalid curlyq #secs: %d", $5);
			dxa.dxa_orbit_secs = $5;

			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_CYCLENC cycle_method {
			INIT_DXA($1);
			dxa.dxa_cycle_meth = $2;;
		}
		| DGT_DMODE STRING {
			int i;

			INIT_DXA($1);
			for (i = 0; i < NDM; i++)
				if (dmodes[i].dm_abbr &&
				    strcasecmp(dmodes[i].dm_abbr, $2) == 0) {
					dxa.dxa_dmode = i;
					break;
				}
			if (i >= NDM)
				yyerror("invalid dmode: %s", $2);
			free($2);
		}
		| DGT_EXIT {
			INIT_DXA($1);
		}
		| DGT_FOCUS dbl COMMA dbl COMMA dbl {
			INIT_DXA($1);
			vec_set(&dxa.dxa_focus, $2, $4, $6);
		}
		| DGT_HL STRING {
			INIT_DXA($1);
			if (strcasecmp($2, "all") == 0)
				dxa.dxa_hl = NC_ALL;
			else if (strcasecmp($2, "seldm") == 0)
				dxa.dxa_hl = NC_SELDM;
			else
				yyerror("invalid hl: %s", $2);
			free($2);
		}
		| DGT_MOVE STRING dbl move_secs {
			INIT_DXA($1);
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

			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_OPT setmodifier opts_l {
			INIT_DXA($1);
			dxa.dxa_opt_mode = $2;
			dxa.dxa_opts = $3;
		}
		| DGT_ORBIT setmodifier dim orbit_revs orbit_secs {
			INIT_DXA($1);
			dxa.dxa_orbit_dir = ($2 == DXV_OFF) ? -1 : 1;
			dxa.dxa_orbit_dim = $3;

			/* Avoid any FPE. */
			if ($4 == 0.0)
				yyerror("invalid orbit #revs: %f", $4);
			dxa.dxa_orbit_frac = fabs($4);

			if ($5 < 0)
				yyerror("invalid orbit #secs: %d", $5);
			dxa.dxa_orbit_secs = $5;

			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_PANEL setmodifier panels_l {
			INIT_DXA($1);
			dxa.dxa_panel_mode = $2;
			dxa.dxa_panels = $3;
		}
		| DGT_PIPEMODE STRING {
			INIT_DXA($1);
			if (strcasecmp($2, "torus") == 0)
				dxa.dxa_pipemode = PM_DIR;
			else if (strcasecmp($2, "rte") == 0)
				dxa.dxa_pipemode = PM_RTE;
			else
				yyerror("invalid pipemode: %s", $2);
			free($2);
		}
		| DGT_PLAYREEL INTG STRING {
			INIT_DXA($1);
			dxa.dxa_reel_delay = $2;
			dxa.dxa_reel = $3;
		}
		| DGT_PSTICK STRING panels_l {
			INIT_DXA($1);
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
			dxa.dxa_panels = $3;
		}
		| DGT_REFOCUS {
			INIT_DXA($1);
			INIT_DXA(DGT_CAMSYNC);
		}
		| DGT_REFRESH {
			INIT_DXA($1);
		}
		| DGT_SELNC STRING {
			INIT_DXA($1);
			if (strcasecmp($2, "random") == 0)
				dxa.dxa_selnc = DXNC_RND;
			else
				yyerror("invalid node class: %s", $2);
			free($2);
		}
		| DGT_SELNODE setmodifier selnode_list {
			INIT_DXA($1);
			dxa.dxa_selnode_mode = $2;
			dxa.dxa_selnode_list = $3;
		}
		| DGT_SETCAP STRING  {
			INIT_DXA($1);
			dxa.dxa_caption = $2;
		}
		| DGT_STALL dbl {
			INIT_DXA($1);
			dxa.dxa_stall_secs = $2;
		}
		| DGT_SSCTL STRING STRING {
			INIT_DXA($1);
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
		}
		| DGT_SUBSEL setmodifier subsel_list {
			INIT_DXA($1);
			dxa.dxa_subsel_mode = $2;
			dxa.dxa_subsel_list = $3;
		}
		| DGT_VMODE STRING {
			INIT_DXA($1);
			if (strcasecmp($2, "phys") == 0)
				dxa.dxa_vmode = VM_PHYS;
			else if (strcasecmp($2, "wired") == 0)
				dxa.dxa_vmode = VM_WIRED;
			else if (strcasecmp($2, "wione") == 0)
				dxa.dxa_vmode = VM_WIONE;
			else
				yyerror("invalid vmode: %s", $2);
			free($2);

			INIT_DXA(DGT_NODESYNC);
		}
		| DGT_WINSP setmodifier INTG setmodifier INTG setmodifier INTG {
			INIT_DXA($1);
			dxa.dxa_winsp_mode.iv_x = $2;
			dxa.dxa_winsp_mode.iv_y = $4;
			dxa.dxa_winsp_mode.iv_z = $6;
			dxa.dxa_winsp.iv_x = $3;
			dxa.dxa_winsp.iv_y = $5;
			dxa.dxa_winsp.iv_z = $7;

			INIT_DXA(DGT_NODESYNC);
		}
		| DGT_WIOFF setmodifier INTG setmodifier INTG setmodifier INTG {
			INIT_DXA($1);
			dxa.dxa_wioff_mode.iv_x = $2;
			dxa.dxa_wioff_mode.iv_y = $4;
			dxa.dxa_wioff_mode.iv_z = $6;
			dxa.dxa_wioff.iv_x = $3;
			dxa.dxa_wioff.iv_y = $5;
			dxa.dxa_wioff.iv_z = $7;

			INIT_DXA(DGT_NODESYNC);
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
