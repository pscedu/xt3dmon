/* $Id$ */

%{

#include "mon.h"

#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "pathnames.h"
#include "phys.h"
#include "mach.h"

#define yyin machin

int		yylex(void);
int		yyerror(const char *, ...);
int		yyparse(void);

struct physdim *physdim_get(const char *);

int		mach_lineno = 1;
static int	errors;

struct physdim	*physdim;
struct physdim	*physdim_top;
LIST_HEAD(, physdim) physdims;

%}

%token CONTAINS
%token COREDIM
%token CUBEDIM
%token DIM
%token MAG
%token NIDMAX
%token OFFSET
%token ORIGIN
%token SIZE
%token SKEL
%token SPACE
%token SPANS

%token COMMA LANGLE LBRACKET RANGLE RBRACKET

%token <string> STRING
%token <intg> INTG
%token <dbl> DBL

%type <dbl> dbl

%union {
	char	*string;
	int	 intg;
	double	 dbl;
}

%%

grammar		: /* empty */
		| grammar conf {
		}
		;

dbl		: INTG		{ $$ = (double)$1; }
		| DBL		{ $$ = $1; }
		;

conf		: DIM STRING LBRACKET {
			physdim = physdim_get($2);
			free($2);
		} dimopts_l RBRACKET {
			if (physdim->pd_mag == 0)
				yyerror("no magnitude specified");
//			if (!physdim->pd_spans)
//				yyerror("no span specified");
		}
		| NIDMAX INTG {
			if ($2 <= 0)
				yyerror("invalid nidmax: %d", $2);
			machine.m_nidmax = $2;
		}
		| ORIGIN LANGLE dbl COMMA dbl COMMA dbl RANGLE {
			vec_set(&machine.m_origin, $3, $5, $7);
		}
		| COREDIM LANGLE INTG COMMA INTG COMMA INTG RANGLE {
			if ($3 <= 0)
				yyerror("invalid core X dimension: %d", $3);
			if ($5 <= 0)
				yyerror("invalid core Y dimension: %d", $5);
			if ($7 <= 0)
				yyerror("invalid core Z dimension: %d", $7);
			ivec_set(&machine.m_coredim, $3, $5, $7);
		}
		| CUBEDIM LANGLE INTG COMMA INTG COMMA INTG RANGLE {
			if ($3 <= 0)
				yyerror("invalid cube X dimension: %d", $3);
			if ($5 <= 0)
				yyerror("invalid cube Y dimension: %d", $5);
			if ($7 <= 0)
				yyerror("invalid cube Z dimension: %d", $7);
			ivec_set(&widim, $3, $5, $7);
		}
		;

dimopts_l	: /* empty */
		| dimopts dimopts_l
		;

dimopts		: MAG INTG {
			physdim->pd_mag = $2;
		}
		| OFFSET LANGLE dbl COMMA dbl COMMA dbl RANGLE {
			physdim->pd_offset.fv_x = $3;
			physdim->pd_offset.fv_y = $5;
			physdim->pd_offset.fv_z = $7;
		}
		| SPACE dbl {
			physdim->pd_space = $2;
		}
		| SPANS STRING {
			if (strcmp($2, "x") == 0)
				physdim->pd_spans = DIM_X;
			else if (strcmp($2, "y") == 0)
				physdim->pd_spans = DIM_Y;
			else if (strcmp($2, "z") == 0)
				physdim->pd_spans = DIM_Z;
			else
				yyerror("%s: invalid span", $2);
			free($2);
		}
		| SKEL STRING {
			if (strcmp($2, "yes") == 0)
				physdim->pd_flags |= PDF_SKEL;
			else if (strcmp($2, "no") != 0)
				yyerror("%s: invalid skel specification", $2);
			free($2);
		}
		| CONTAINS STRING {
			struct physdim *pd;

			pd = physdim_get($2);
			if (pd == physdim)
				yyerror("%s: dimension cannot contain itself", $2);
			physdim->pd_contains = pd;
			pd->pd_containedby = physdim;
			LIST_REMOVE(pd, pd_link);
			free($2);
		}
		| SIZE LANGLE dbl COMMA dbl COMMA dbl RANGLE {
			physdim->pd_size.fv_x = $3;
			physdim->pd_size.fv_y = $5;
			physdim->pd_size.fv_z = $7;
		}
		;

%%

int
yyerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", progname, mach_lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);

	errors++;
	return (0);
}

void
physdim_check(void)
{
	struct physdim *pd, *spd, *lpd;

	if (LIST_EMPTY(&physdims))
		yyerror("no dimensions specified");
	else {
		pd = LIST_FIRST(&physdims);
		if (LIST_NEXT(pd, pd_link))
			yyerror("%s: incoherent dimension", pd->pd_name);
		else {
			lpd = spd = pd;
			while ((spd = spd->pd_contains) != NULL) {
				if (spd == pd) {
					yyerror("loop detected");
					return;
				}
				lpd = spd;
			}

			/*
			 * Now propagate spacing measurements up through
			 * the dimensions.
			 */
			pd = lpd;
			for (; pd != NULL && pd->pd_containedby != NULL;
			    pd = spd) {
				spd = pd->pd_containedby;
				if (!vec_eq(&spd->pd_size, &fv_zero))
					continue;
				spd->pd_size = pd->pd_size;
				spd->pd_size.fv_val[pd->pd_spans] =
				    pd->pd_size.fv_val[pd->pd_spans] * pd->pd_mag +
				    pd->pd_space * (pd->pd_mag - 1);
				spd->pd_size.fv_x += pd->pd_offset.fv_x * (pd->pd_mag - 1);
				spd->pd_size.fv_y += pd->pd_offset.fv_y * (pd->pd_mag - 1);
				spd->pd_size.fv_z += pd->pd_offset.fv_z * (pd->pd_mag - 1);
			}
		}
	}
}

void
parse_machconf(const char *fn)
{
	extern FILE *yyin;
	struct physdim *pd;
	FILE *fp;

	LIST_INIT(&physdims);
	if ((fp = fopen(fn, "r")) == NULL)
		err(1, "%s", fn);
	yyin = fp;
	mach_lineno = 1;
	yyparse();
	fclose(fp);

	physdim_check();
	physdim_top = LIST_FIRST(&physdims);

	if (errors)
		errx(1, "%d error(s) encountered", errors);

	LIST_FOREACH(pd, &physdims, pd_link)
		machine.m_nphysdim++;

	vec_copyto(&physdim_top->pd_size, &machine.m_dim);
	/*
	 * XXX just push another physdim atop physdim_top
	 * to reuse code from physdim_check().
	 *
	 * XXX respect pd_offset
	 */
	machine.m_dim.fv_val[physdim_top->pd_spans] =
	    physdim_top->pd_size.fv_val[physdim_top->pd_spans] * physdim_top->pd_mag +
	    physdim_top->pd_space * (physdim_top->pd_mag - 1) - 38;

	vec_set(&machine.m_center,
	    machine.m_origin.fv_x + machine.m_dim.fv_w / 2.0,
	    machine.m_origin.fv_y + machine.m_dim.fv_h / 2.0,
	    machine.m_origin.fv_z + machine.m_dim.fv_d / 2.0);
}

struct physdim *
physdim_get(const char *name)
{
	struct physdim *pd, *spd;

	LIST_FOREACH(pd, &physdims, pd_link) {
		if (strcmp(pd->pd_name, name) == 0)
			return (pd);
		spd = pd;
		while ((spd = spd->pd_contains))
			if (strcmp(pd->pd_name, name) == 0)
				return (pd);
	}
	if ((pd = malloc(sizeof(*pd))) == NULL)
		err(1, "malloc");
	memset(pd, 0, sizeof(*pd));
	if ((pd->pd_name = strdup(name)) == NULL)
		err(1, "strdup");
	LIST_INSERT_HEAD(&physdims, pd, pd_link);
	return (pd);
}
