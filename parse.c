/* $Id$ */

#if defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE	/* asprintf */
#endif

#include <sys/param.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mon.h"

#define MAXFAILS	200
#define JINCR		10
#define FINCR		10
#define TINCR		10

void		 parse_badmap(void);
void		 parse_checkmap(void);
void		 parse_qstat(void);
void		 getcol(int, size_t, struct fill *);
void		*getobj(void *, struct objlist *);

int		 job_eq(void *, void *);
int		 fail_eq(void *, void *);
int		 temp_eq(void *, void *);

struct objlist	 job_list  = { { NULL }, 0, 0, 0, 0, JINCR, sizeof(struct job), job_eq };
struct objlist	 temp_list = { { NULL }, 0, 0, 0, 0, TINCR, sizeof(struct temp), fail_eq };
struct objlist	 fail_list = { { NULL }, 0, 0, 0, 0, FINCR, sizeof(struct fail), temp_eq };

int		 total_failures;
unsigned long	 vmem;
long		 rmem;

struct fail fail_notfound = {
	{ 0, 0 }, 0, { 0.33f, 0.66f, 0.99f, 1.00f, 0, 0 }, "0"
};

struct temp temp_notfound = {
	{ 0, 0 }, 0, { 0.00f, 0.00f, 0.00f, 1.00f, 0, 0 }, "?"
};

struct temp_range temp_map[] = {
	{ { 0.0f, 0.0f, 0.4f, 1.0f, 0, 0 }, "18-22C" },
	{ { 0.8f, 0.0f, 0.4f, 1.0f, 0, 0 }, "22-26C" },
	{ { 0.6f, 0.0f, 0.6f, 1.0f, 0, 0 }, "26-31C" },
	{ { 0.4f, 0.0f, 0.8f, 1.0f, 0, 0 }, "31-35C" },
	{ { 0.2f, 0.2f, 1.0f, 1.0f, 0, 0 }, "35-40C" },
	{ { 0.0f, 0.0f, 1.0f, 1.0f, 0, 0 }, "40-44C" },
	{ { 0.0f, 0.6f, 0.6f, 1.0f, 0, 0 }, "44-49C" },
	{ { 0.0f, 0.8f, 0.0f, 1.0f, 0, 0 }, "49-53C" },
	{ { 0.4f, 1.0f, 0.0f, 1.0f, 0, 0 }, "53-57C" },
	{ { 1.0f, 1.0f, 0.0f, 1.0f, 0, 0 }, "57-62C" },
	{ { 1.0f, 0.8f, 0.2f, 1.0f, 0, 0 }, "62-66C" },
	{ { 1.0f, 0.6f, 0.0f, 1.0f, 0, 0 }, "66-71C" },
	{ { 1.0f, 0.0f, 0.0f, 1.0f, 0, 0 }, "71-75C" },
	{ { 1.0f, 0.6f, 0.6f, 1.0f, 0, 0 }, "75-80C" }
};

int
job_eq(void *elem, void *arg)
{
	return (((struct job *)elem)->j_id == *(int *)arg);
}

int
temp_eq(void *elem, void *arg)
{
	return (((struct temp *)elem)->t_cel == *(int *)arg);
}

int
fail_eq(void *elem, void *arg)
{
	return (((struct fail *)elem)->f_fails == *(int *)arg);
}

#define CMP(a, b) ((a) < (b) ? -1 : ((a) == (b) ? 0 : 1))

int
temp_cmp(const void *a, const void *b)
{
	return (CMP((*(struct temp **)a)->t_cel, (*(struct temp **)b)->t_cel));
}

int
fail_cmp(const void *a, const void *b)
{
	return (CMP((*(struct fail **)a)->f_fails, (*(struct fail **)b)->f_fails));
}

void
obj_batch_start(struct objlist *ol)
{
	struct objhdr *ohp;
	void **jj;
	size_t n;

	ol->ol_tcur = 0;
	if (ol->ol_data == NULL)
		return;
	for (n = 0, jj = ol->ol_data; n < ol->ol_cur; jj++, n++) {
		ohp = (struct objhdr *)*jj;
		ohp->oh_tref = 0;
	}
}

void
obj_batch_end(struct objlist *ol)
{
	struct objhdr *ohp, *swapohp;
	size_t n, lookpos;
	void *t, **jj;

	ol->ol_cur = ol->ol_tcur;
	if (ol->ol_tcur > ol->ol_max)
		ol->ol_max = ol->ol_tcur;
	if (ol->ol_data == NULL)
		return;
	lookpos = 0;
	for (n = 0, jj = ol->ol_data; n < ol->ol_cur; jj++, n++) {
		ohp = (struct objhdr *)*jj;
		if (ohp->oh_tref)
			ohp->oh_ref = 1;
		else {
			if (lookpos <= n)
				lookpos = n + 1;
			/* Scan forward to swap. */
			for (; lookpos < ol->ol_cur; lookpos++) {
				swapohp = ol->ol_data[lookpos];
				if (swapohp->oh_tref) {
					t = ol->ol_data[n];
					ol->ol_data[n] = ol->ol_data[lookpos];
					ol->ol_data[lookpos++] = t;
					break;
				}
			}
			if (lookpos == ol->ol_cur) {
				ol->ol_cur = n;
				return;
			}
		}
	}
}

/*
 * The "eq" routines must ensure that an object identifier can never be
 * zero; otherwise, they will match empty objects after memset().
 */
void *
getobj(void *arg, struct objlist *ol)
{
	void **jj, *j = NULL;
	struct objhdr *ohp;
	size_t n, max;

	if (ol->ol_data != NULL) {
		max = MAX(ol->ol_max, ol->ol_tcur);
		for (n = 0, jj = ol->ol_data; n < max; jj++, n++) {
			ohp = (struct objhdr *)*jj;
			if (ol->ol_eq(ohp, arg))
				goto found;
		}
	}
	/* Not found; add. */
	if (ol->ol_tcur + 1 >= ol->ol_alloc) {
		max = ol->ol_alloc + ol->ol_incr;
		if ((ol->ol_data = realloc(ol->ol_data,
		    max * sizeof(*ol->ol_data))) == NULL)
			err(1, "realloc");
		for (n = ol->ol_alloc; n < max; n++) {
			if ((j = malloc(ol->ol_objlen)) == NULL)
				err(1, "malloc");
			memset(j, 0, ol->ol_objlen);
			ol->ol_data[n] = j;
		}
		ol->ol_alloc = max;
	}
	ohp = ol->ol_data[ol->ol_tcur];
found:
	if (!ohp->oh_tref) {
		ol->ol_tcur++;
		ohp->oh_tref = 1;
	}
	return (ohp);
}

void
getcol(int n, size_t total, struct fill *fillp)
{
	double div;

	if (total == 1)
		div = 0.0;
	else
		div = n / (double)(total - 1);

	fillp->f_r = cos(div);
	fillp->f_g = sin(div) * sin(div);
	fillp->f_b = fabs(tan(div + PI * 3/4));
	fillp->f_a = 1.0;
}

void
getcol_temp(int n, struct fill *fillp)
{
	int cel = temp_list.ol_temps[n]->t_cel;
	int idx;

	if (cel < TEMP_MIN)
		cel = TEMP_MIN;
	else if (cel > TEMP_MAX)
		cel = TEMP_MAX;

	idx = (cel - TEMP_MIN) * TEMP_NTEMPS / (TEMP_MAX - TEMP_MIN);

	*fillp = temp_map[idx].m_fill;
}

/*
 * Example line:
 *	33     c0-0c1s0s1      0,5,0
 *
 * Broken up:
 *	nid	cb r cb  m  n	x y z
 *	33	c0-0 c1 s0 s1	0,5,0
 */
void
parse_physmap(void)
{
	int lineno, r, cb, cg, m, n, nid, x, y, z;
	char buf[BUFSIZ], *p, *s;
	struct node *node;
	FILE *fp;
	long l;

	wired_width = wired_height = wired_depth = 0;

	/* Explicitly initialize all nodes. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_state = JST_UNACC;
						node->n_fillp = &jstates[JST_UNACC].js_fill;
						node->n_hide = 1;
					}

	if ((fp = fopen(_PATH_PHYSMAP, "r")) == NULL) {
		warn("%s", _PATH_PHYSMAP);
		return;
	}
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;

		/* nid */
		if (!isdigit(*p))
			goto bad;
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NID_MAX)
			goto bad;
		nid = (int)l;

		/* cb */
		p = s;
		while (isspace(*p))
			p++;
		if (*p++ != 'c')
			goto bad;
		if (!isdigit(*p))
			goto bad;
		s = p;
		while (isdigit(*++s))
			;
		if (*s != '-')
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NCABS)
			goto bad;
		cb = (int)l;

		/* r */
		p = s + 1;
		s = p;
		while (isdigit(*++s))
			;
		if (*s != 'c')
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NROWS)
			goto bad;
		r = (int)l;

		/* cg */
		p = s + 1;
		s = p;
		while (isdigit(*++s))
			;
		if (*s != 's')
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NCAGES)
			goto bad;
		cg = (int)l;

		/* m */
		p = s;
		while (isdigit(*++s))
			;
		if (*s != 's')
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NMODS)
			goto bad;
		m = (int)l;

		/* n */
		p = s;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l >= NNODES)
			goto bad;
		n = (int)l;

		/* x */
		while (isspace(*s))
			s++;
		if (!isdigit(*s))
			goto bad;
		p = s;
		while (isdigit(*++s))
			;
		if (*s != ',')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
			goto bad;
		x = (int)l;

		/* y */
		while (isspace(*s))
			s++;
		if (!isdigit(*s))
			goto bad;
		p = s;
		while (isdigit(*++s))
			;
		if (*s != ',')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
			goto bad;
		y = (int)l;

		/* z */
		while (isspace(*s))
			s++;
		if (!isdigit(*s))
			goto bad;
		p = s;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
			goto bad;
		z = (int)l;

		/* done */

		node = &nodes[r][cb][cg][m][n];
		node->n_nid = nid;
		invmap[nid] = node;
		node->n_wiv.v_x = x;
		node->n_wiv.v_y = y;
		node->n_wiv.v_z = z;

		if (x > wired_width)
			wired_width = x;
		if (y > wired_height)
			wired_height = y;
		if (z > wired_depth)
			wired_depth = z;

		/* state */
		while (isspace(*s))
			s++;
		if (!isalpha(*s))
			goto bad;
		switch (*s) {
		case 'c': /* compute */
			node->n_state = JST_FREE;	/* Good enough. */
			break;
		case 'n': /* disabled */
			node->n_state = JST_DISABLED;
			break;
		case 'i': /* I/O */
			node->n_state = JST_IO;
			break;
		default:
			goto bad;
		}
		node->n_fillp = &jstates[node->n_state].js_fill;
		node->n_hide = 0;
		continue;
bad:
		warnx("%s:%d: malformed line [%s] [%s]",
		    _PATH_PHYSMAP, lineno, buf, p);
	}
	if (ferror(fp))
		warn("%s", _PATH_PHYSMAP);
	fclose(fp);
	errno = 0;

	wired_width++;
	wired_height++;
	wired_depth++;
}

/*
 * Example:
 *	40 1 3661
 *
 * Broken down:
 *	nid enabled jobid
 *	40  1       3661
 *
 * Bad/check format:
 *	nid disabled (if non-zero)
 *	40  1
 */
void
parse_jobmap(void)
{
	int jobid, nid, lineno, enabled;
	char buf[BUFSIZ], *p, *s;
	struct node *node;
	struct job *job;
	size_t j;
	FILE *fp;
	long l;

	/* XXXXXX - reset fillp on all nodes. */

	if ((fp = fopen(_PATH_JOBMAP, "r")) == NULL) {
		warn("%s", _PATH_JOBMAP);
		return;
	}
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;
		if (!isdigit(*p))
			goto bad;

		/* nid */
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
			goto bad;
		nid = (int)l;

		/* enabled */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		enabled = (int)l;

		/* job id */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		jobid = (int)l;

		if ((node = node_for_nid(nid)) == NULL) {
			if (enabled)
				warnx("inconsistency: node %d should be "
				    "disabled in jobmap", nid);
			goto pass;
		}

		if (enabled == 0)
			node->n_state = JST_DOWN;
		else if (jobid == 0)
			node->n_state = JST_FREE;
		else {
			node->n_state = JST_USED;
			job = getobj(&jobid, &job_list);
			job->j_id = jobid;
			/* XXX: only slightly sloppy. */
			job->j_fill.f_texid = jstates[JST_USED].js_fill.f_texid;
			node->n_job = job;
			node->n_fillp = &job->j_fill;
		}

		if (node->n_state != JST_USED)
			node->n_fillp = &jstates[node->n_state].js_fill;
		continue;
bad:
		warn("%s:%d: malformed line", _PATH_JOBMAP, lineno);
pass:
		; //node->n_fillp = ;
	}
	if (ferror(fp))
		warn("%s", _PATH_JOBMAP);
	fclose(fp);

	parse_badmap();
	parse_checkmap();
	parse_qstat();
	errno = 0;

	for (j = 0; j < job_list.ol_tcur; j++)
		getcol(j, job_list.ol_tcur, &job_list.ol_jobs[j]->j_fill);
}

void
parse_badmap(void)
{
	char *s, *p, buf[BUFSIZ];
	int lineno, bad, nid;
	struct node *n;
	FILE *fp;
	long l;

	if ((fp = fopen(_PATH_BADMAP, "r")) == NULL)
		return;						/* Failure is OK. */
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;
		if (!isdigit(*p))
			goto bad;

		/* nid */
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
			goto bad;
		nid = (int)l;

		/* disabled */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		bad = (int)l;

		if (bad) {
			if ((n = node_for_nid(nid)) == NULL)
				goto bad;
			n->n_state = JST_BAD;
			n->n_fillp = &jstates[JST_BAD].js_fill;
		}
		continue;
bad:
		warnx("%s:%d: malformed line", _PATH_BADMAP, lineno);
	}
	if (ferror(fp))
		warn("%s", _PATH_BADMAP);
	fclose(fp);
}

void
parse_checkmap(void)
{
	char *s, *p, buf[BUFSIZ];
	int lineno, checking, nid;
	struct node *n;
	FILE *fp;
	long l;

	if ((fp = fopen(_PATH_CHECKMAP, "r")) == NULL)
		return;						/* Failure is OK. */
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		if (*p == '#')
			continue;
		if (!isdigit(*p))
			goto bad;

		/* nid */
		s = p;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
			goto bad;
		nid = (int)l;

		/* disabled */
		p = s + 1;
		s = p;
		if (!isdigit(*s))
			goto bad;
		while (isdigit(*++s))
			;
		if (!isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
			goto bad;
		checking = (int)l;

		if (checking) {
			if ((n = node_for_nid(nid)) == NULL)
				goto bad;
			n->n_state = JST_CHECK;
			n->n_fillp = &jstates[JST_BAD].js_fill;
		}
		continue;
bad:
		warnx("%s:%d: malformed line", _PATH_CHECKMAP, lineno);
	}
	if (ferror(fp))
		warn("%s", _PATH_CHECKMAP);
	fclose(fp);
}

/*
 * Parse failure data entries.
 *
 * Format:
 *	1		2724
 *	<nfailures>	<nid>
 *
 * Notes:
 *	zero data is not listed.
 */
void
parse_failmap(void)
{
	int nid, lineno, r, cb, cg, m, n;
	size_t j, newmax, nofails;
	char *p, *s, buf[BUFSIZ];
	struct node *node;
	struct fail *fail;
	FILE *fp;
	long l;

	if ((fp = fopen(_PATH_FAILMAP, "r")) == NULL) {
		warn("%s", _PATH_FAILMAP);
		return;
	}

	/*
	 * Because entries with zero failures are not listed,
	 * we must go through and reset all entries.
	 */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_fail = &fail_notfound;
						node->n_fillp = &fail_notfound.f_fill;
					}

	total_failures = newmax = 0;
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;
		while (isspace(*p))
			p++;
		for (s = p; isdigit(*s); s++)
			;
		if (s == p || !isspace(*s))
			goto bad;
		*s = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
			goto bad;
		nofails = (int)l;

		p = s + 1;
		while (isspace(*p))
			p++;
		for (s = p; isdigit(*s); s++)
			;
		if (s == p)
			goto bad;
		*s = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NID_MAX)
			goto bad;
		nid = (int)l;

		if (nofails > MAXFAILS)
			nofails = MAXFAILS;

		if ((node = node_for_nid(nid)) == NULL)
			goto bad;
		fail = getobj(&nofails, &fail_list);
		fail->f_fails = nofails;
		free(fail->f_name);		/* XXX - rename to f_label */
		if (asprintf(&fail->f_name, "%d", nofails) == -1)
			err(1, "asprintf");
		node->n_fillp = &fail->f_fill;
		node->n_fail = fail;

		/* Compute failure statistics. */
		total_failures += nofails;
		if (nofails > newmax)
			newmax = nofails;
		continue;
bad:
		warnx("%s:%d: malformed line: %s [s: %s, p: %s, l:%ld]",
		    _PATH_FAILMAP, lineno, buf, s, p, l);
	}
	if (ferror(fp))
		warn("%s", _PATH_FAILMAP);
	fclose(fp);
	errno = 0;

	qsort(fail_list.ol_fails, fail_list.ol_tcur, sizeof(struct fail *),
	    fail_cmp);
	for (j = 0; j < fail_list.ol_tcur; j++)
		getcol(j, fail_list.ol_tcur, &fail_list.ol_fails[j]->f_fill);
}

/*
 * Temperature data.
 *
 * Example:
 *	cx0y0c0s4	    20  18  18  19
 *	position	[[[[t1] t2] t3] t4]
 */
void
parse_tempmap(void)
{
	int t, lineno, i, r, cb, cg, m, n;
	char buf[BUFSIZ], *p, *s;
	struct node *node;
	struct temp *temp;
	size_t j;
	FILE *fp;
	long l;

	if ((fp = fopen(_PATH_TEMPMAP, "r")) == NULL) {
		warn("%s", _PATH_TEMPMAP);
		return;
	}

	/*
	 * We're not guarenteed to have temperature information for
	 * every node...
	 */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_temp = &temp_notfound;
						node->n_fillp = &temp_notfound.t_fill;
					}

	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		p = buf;

		while (isspace(*p))
			p++;
		if (*p == '#' || *p == '\n' || *p == '\0')
			continue;
		if (*p++ != 'c')
			goto bad;
		if (*p++ != 'x')
			goto bad;

		/* cab */
		if (!isdigit(*p))
			goto bad;
		for (s = p + 1; isdigit(*s); s++)
			;
		if (*s != 'y')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NCABS)
			goto bad;
		cb = (int)l;

		/* row */
		p = s;
		while (isdigit(*s))
			s++;
		if (p == s)
			goto bad;
		if (*s != 'c')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NROWS)
			goto bad;
		r = (int)l;

		/* cage */
		p = s;
		while (isdigit(*s))
			s++;
		if (p == s)
			goto bad;
		if (*s != 's')
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NCAGES)
			goto bad;
		cg = (int)l;

		/* mod */
		p = s;
		while (isdigit(*s))
			s++;
		if (p == s)
			goto bad;
		if (!isspace(*s))
			goto bad;
		*s++ = '\0';
		if ((l = strtol(p, NULL, 10)) < 0 || l >= NMODS)
			goto bad;
		m = (int)l;

		/* temperatures */
		for (i = 0; i < NNODES; i++) {
			while (isspace(*s))
				s++;
			if (*s == '\0')
				break;
			p = s;
			while (isdigit(*s))
				s++;
			if (p == s)
				goto bad;
			if (!isspace(*s))
				goto bad;
			*s++ = '\0';
			if ((l = strtol(p, NULL, 10)) < 0 || l >= INT_MAX)
				goto bad;
			t = (int)l;

			node = &nodes[r][cb][cg][m][i];
			temp = getobj(&t, &temp_list);
			temp->t_cel = t;
			free(temp->t_name);	/* getobj() zeroes data. */
			if (asprintf(&temp->t_name, "%dC", t) == -1)
				err(1, "asprintf");
			node->n_fillp = &temp->t_fill;
			node->n_temp = temp;
		}
		continue;
bad:
		warnx("%s:%d: malformed line; %s", _PATH_TEMPMAP, lineno, p);
	}
	if (ferror(fp))
		warn("%s", _PATH_TEMPMAP);
	fclose(fp);
	errno = 0;

	qsort(temp_list.ol_temps, temp_list.ol_tcur, sizeof(struct temp *),
	    temp_cmp);
	for (j = 0; j < temp_list.ol_tcur; j++)
		getcol_temp(j, &temp_list.ol_temps[j]->t_fill);
}

#define _PATH_STAT "/proc/self/stat"

void
parse_mem(void)
{
	FILE *fp;

	if ((fp = fopen(_PATH_STAT, "r")) == NULL) {
		warn("%s", _PATH_STAT);
		return;
	}
	fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
	    "%*u %*u %*d %*d %*d %*d %*d %*d %*u %lu %ld", &vmem, &rmem);
	fclose(fp);
}

#define _PATH_QSTAT "data/qstat.out"

/*
 * Job Id: 4864.phantom.psc.edu
 *   Job_Name = run.cpmd
 *   Job_Owner = stbrown@phantom.psc.edu
 *   resources_used.cput = 00:00:00
 *   resources_used.mem = 11156kb
 *   resources_used.vmem = 29448kb
 *   resources_used.walltime = 02:38:03
 *   job_state = R
 */
void
parse_qstat(void)
{
	char state, *t, *s, *q, buf[BUFSIZ], *next;
	struct job j_fake, *job;
	int jobid;
	FILE *fp;

	if ((fp = fopen(_PATH_QSTAT, "r")) == NULL) {
		warn("%s", _PATH_QSTAT);
		return;
	}
	jobid = 0;
	state = '\0';
	for (;;) {
		next = fgets(buf, sizeof(buf), fp);
		if (next == NULL && ferror(fp)) {
			warn("%s", _PATH_QSTAT);
			break;
		}

		q = "Job Id: ";
		if (next == NULL || (s = strstr(buf, q)) != NULL) {
			/* Save last job info. */
			if (state == 'R' &&
			    (job = job_findbyid(jobid)) != NULL) {
				job->j_ncpus = j_fake.j_ncpus;
				job->j_tmdur = j_fake.j_tmdur;
				job->j_tmuse = j_fake.j_tmuse;
				memcpy(job->j_jname, j_fake.j_jname,
				    sizeof(job->j_jname));
				memcpy(job->j_owner, j_fake.j_owner,
				    sizeof(job->j_owner));
				memcpy(job->j_queue, j_fake.j_queue,
				    sizeof(job->j_queue));
			}

			/* Setup next job. */
			jobid = 0;
			state = 0;
			j_fake.j_jname[0] = '\0';
			j_fake.j_owner[0] = '\0';
			j_fake.j_queue[0] = '\0';
			j_fake.j_tmdur = 0;
			j_fake.j_tmuse = 0;
			j_fake.j_ncpus = 0;

			if (feof(fp))
				break;
			else {
				s += strlen(q);
				jobid = strtoul(s, NULL, 0);
			}
		}
		/* buf should always be valid here. */
		if ((t = strchr(buf, '\n')) != NULL)
			*t = '\0';

		q = "Job_Name = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			strncpy(j_fake.j_jname, s,
			    sizeof(j_fake.j_jname) - 1);
			j_fake.j_jname[sizeof(j_fake.j_jname) - 1] = '\0';
		}

		q = "Job_Owner = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			strncpy(j_fake.j_owner, s,
			    sizeof(j_fake.j_owner) - 1);
			j_fake.j_owner[sizeof(j_fake.j_owner) - 1] = '\0';
		}

		q = "job_state = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			state = *s;
		}

		q = "queue = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			strncpy(j_fake.j_queue, s,
			    sizeof(j_fake.j_queue) - 1);
			j_fake.j_queue[sizeof(j_fake.j_queue) - 1] = '\0';
		}

		q = "Resource_List.size = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			j_fake.j_ncpus = strtoul(s, NULL, 0);
		}

		q = "Resource_List.walltime = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			if ((t = strchr(s, ':')) != NULL)
				*t++ = '\0';
			j_fake.j_tmdur = 60 * strtoul(s, NULL, 0);
			j_fake.j_tmdur += strtoul(t, NULL, 0);
		}

		q = "resources_used.walltime = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			if ((t = strchr(s, ':')) != NULL)
				*t++ = '\0';
			j_fake.j_tmuse = 60 * strtoul(s, NULL, 0);
			j_fake.j_tmuse += strtoul(t, NULL, 0);
		}
	}
	fclose(fp);
}
