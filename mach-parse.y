/* $Id$ */

%{

#include <err.h>
#include <stdarg.h>

#include "phys.h"
#include "mon.h"

int yylex(void);
int yyerror(const char *, ...);

int lineno = 1;
static int errors;

struct physdim *physdim;
struct physdim_hd physdims;

%}

%token CONTAINS DIM MAG OFFSET SIZE SKEL SPACE SPANS
%token COMMA LANGLE LBRACKET RANGLE RBRACKET

%token <string> STRING
%token <wnumber> WNUMBER
%token <fnumber> FNUMBER

%union {
	char		*string;
	int		 wnumber;
	double		 fnumber;
}

%%

grammar		: /* empty */
		| grammar conf {
		}
		;

conf		: DIM STRING LBRACKET {
			physdim = physdim_get($2);
			free($2);
		} opts_l RBRACKET {
			if (physdim->pd_mag == 0)
				yyerror("no magnitude specified");
//			if (!physdim->pd_spans)
//				yyerror("no span specified");
		}
		;

opts_l		: /* empty */
		| opts opts_l
		;

opts		: MAG WNUMBER {
			physdim->pd_mag = $2;
		}
		| OFFSET FNUMBER {
			physdim->pd_offset = $2;
		}
		| SPACE WNUMBER {
			physdim->pd_space = $2;
		}
		| SPACE FNUMBER {
			physdim->pd_space = $2;
		}
		| SPANS STRING {
			if (strcmp($2, "x"))
				physdim->pd_spans = DIM_X;
			else if (strcmp($2, "y"))
				physdim->pd_spans = DIM_Y;
			else if (strcmp($2, "z"))
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
		| SIZE LANGLE FNUMBER COMMA FNUMBER COMMA FNUMBER RANGLE {
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
	fprintf(stderr, "%s:%d: ", progname, lineno);
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
	struct fvec fv;
	float *pv, *sv;

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
			fv = lpd->pd_size;
			pd = lpd;
			for (; pd != NULL && pd->pd_containedby != NULL;
			    pd = spd) {
				spd = pd->pd_containedby;
				spd->pd_size = pd->pd_size;
				switch (pd->pd_spans) {
				case DIM_X:
					pv = &pd->pd_size.fv_x;
					sv = &spd->pd_size.fv_x;
					break;
				case DIM_Y:
					pv = &pd->pd_size.fv_y;
					sv = &spd->pd_size.fv_y;
					break;
				case DIM_Z:
					pv = &pd->pd_size.fv_z;
					sv = &spd->pd_size.fv_z;
					break;
				}
				*sv = pd->pd_mag * (*pv + 2 * pd->pd_space) +
				    pd->pd_offset * (spd->pd_mag - 1);
			}
		}
	}
}

void
parse_physconf(void)
{
	FILE *fp;
	extern FILE *yyin;

	LIST_INIT(&physdims);
	if ((fp = fopen(_PATH_PHYSCONF, "r")) == NULL)
		err(1, "%s", _PATH_PHYSCONF);
	yyin = fp;
	yyparse();
	fclose(fp);

	physdim_check();

	if (errors)
		errx(1, "%d error(s) encountered", errors);
exit(0);
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
