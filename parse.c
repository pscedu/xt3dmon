/* $Id$ */

#ifdef __GNUC__
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

typedef int (*cmpf_t)(void *a, void *b);

void		 parse_badmap(void);
void		 parse_checkmap(void);
void		 getcol(int, size_t, struct fill *);
void		*getobj(void *arg, void ***data, size_t oldsiz, size_t *newsiz,
    size_t *maxsiz, cmpf_t eq, int inc, size_t);

size_t		 njobs, maxjobs;
size_t		 nfails, maxfails;
size_t		 ntemps, maxtemps;

struct fail	**fails;
struct job	**jobs;
struct temp	**temps;

int		 total_failures;

struct fail fail_notfound = {
	{ 0, 0 }, 0, { 0.33f, 0.66f, 0.99f, 1.00f, 0, 0 }, "0"
};

struct temp temp_notfound = {
	{ 0, 0 }, 0, { 0.00f, 0.00f, 0.00f, 1.00f, 0, 0 }, "?"
};

struct temp_range temp_map[] = {
	{ { 0.0f, 0.0f, 0.4f, 1.0f, 0, 0 }, "18-20C" },
	{ { 0.8f, 0.0f, 0.4f, 1.0f, 0, 0 }, "21-23C" },
	{ { 0.6f, 0.0f, 0.6f, 1.0f, 0, 0 }, "24-26C" },
	{ { 0.4f, 0.0f, 0.8f, 1.0f, 0, 0 }, "27-29C" },
	{ { 0.2f, 0.2f, 1.0f, 1.0f, 0, 0 }, "30-32C" },
	{ { 0.0f, 0.0f, 1.0f, 1.0f, 0, 0 }, "33-35C" },
	{ { 0.0f, 0.6f, 0.6f, 1.0f, 0, 0 }, "36-38C" },
	{ { 0.0f, 0.8f, 0.0f, 1.0f, 0, 0 }, "39-41C" },
	{ { 0.4f, 1.0f, 0.0f, 1.0f, 0, 0 }, "42-44C" },
	{ { 1.0f, 1.0f, 0.0f, 1.0f, 0, 0 }, "45-47C" },
	{ { 1.0f, 0.8f, 0.2f, 1.0f, 0, 0 }, "48-50C" },
	{ { 1.0f, 0.6f, 0.0f, 1.0f, 0, 0 }, "51-53C" },
	{ { 1.0f, 0.0f, 0.0f, 1.0f, 0, 0 }, "54-56C" },
	{ { 1.0f, 0.6f, 0.6f, 1.0f, 0, 0 }, "57-59C" }
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
obj_batch_start(void ***data, size_t cursiz)
{
	struct objhdr *ohp;
	void **jj;
	size_t n;

	if (*data == NULL)
		return;
	for (n = 0, jj = *data; n < cursiz; jj++, n++) {
		ohp = (struct objhdr *)*jj;
		ohp->oh_tref = 0;
	}
}

void
obj_batch_end(void ***data, size_t *cursiz)
{
	struct objhdr *ohp, *swapohp;
	size_t n, lookpos;
	void *t, **jj;

	if (*data == NULL)
		return;
	lookpos = 0;
	for (n = 0, jj = *data; n < *cursiz; jj++, n++) {
		ohp = (struct objhdr *)*jj;
		if (ohp->oh_tref)
			ohp->oh_ref = 1;
		else {
			if (lookpos <= n)
				lookpos = n + 1;
			/* Scan forward to swap. */
			for (; lookpos < *cursiz; lookpos++) {
				swapohp = (*data)[lookpos];
				if (swapohp->oh_tref) {
					t = (*data)[n];
					(*data)[n] = (*data)[lookpos];
					(*data)[lookpos++] = t;
					break;
				}
			}
			if (lookpos == *cursiz) {
				*cursiz = n;
				return;
			}
		}
	}
}

void *
getobj(void *arg, void ***data, size_t oldsiz, size_t *newsiz, size_t *maxsiz,
    cmpf_t eq, int inc, size_t objlen)
{
	size_t n, newmax;
	void **jj, *j = NULL;
	struct objhdr *ohp;

	if (*data != NULL)
		for (n = 0, jj = *data; n < oldsiz; jj++, n++) {
			ohp = (struct objhdr *)*jj;
			if (eq(ohp, arg))
				goto found;
		}
	/* Not found; add. */
	if (*newsiz + 1 >= *maxsiz) {
		newmax = *maxsiz + inc;
		if ((*data = realloc(*data,
		    newmax * sizeof(**data))) == NULL)
			err(1, "realloc");
		for (n = *maxsiz; n < newmax; n++) {
			if ((j = malloc(objlen)) == NULL)
				err(1, "malloc");
			memset(j, 0, objlen);
			(*data)[n] = j;
		}
		*maxsiz = newmax;
	}
	ohp = (*data)[(*newsiz)++];
found:
	ohp->oh_tref = 1;
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
	int cel = temps[n]->t_cel;
	int idx;

	if (cel < TEMP_MIN)
		cel = TEMP_MIN;
	else if (cel > TEMP_MAX)
		cel = TEMP_MAX;

	idx = (cel - TEMP_MIN) / ((TEMP_MAX - TEMP_MIN) / TEMP_NTEMPS);

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
	size_t j, newnjobs;
	struct node *node;
	struct job *job;
	FILE *fp;
	long l;

	/* XXXXXX - reset fillp on all nodes. */

	if ((fp = fopen(_PATH_JOBMAP, "r")) == NULL) {
		warn("%s", _PATH_JOBMAP);
		return;
	}
	lineno = 0;
	newnjobs = 0;
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
			job = getobj(&jobid, (void ***)&jobs, njobs,
			    &newnjobs, &maxjobs, job_eq, JINCR,
			    sizeof(*job));
			job->j_id = jobid;
			job->j_name = "testjob";
			/* XXX: only slightly sloppy. */
			job->j_fill.f_texid = jstates[JST_USED].js_fill.f_texid;
			// free(j->j_name);
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
	errno = 0;

	/* XXX XXX: for some reason newnjobs is zero the
	second time and is screwing things up */
	if(newnjobs != 0)
		njobs = newnjobs;
printf("parse_jobmap - njobs: %d\n", njobs);
	for (j = 0; j < njobs; j++)
		getcol(j, njobs, &jobs[j]->j_fill);
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
	size_t j, newmax, nofails, newnfails;
	int nid, lineno, r, cb, cg, m, n;
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

	newnfails = 0;
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
		fail = getobj(&nofails, (void ***)&fails, nfails,
		    &newnfails, &maxfails, fail_eq, FINCR, sizeof(struct fail));
		fail->f_fails = nofails;
		free(fail->f_name); /* XXX - rename to f_label */
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

	nfails = newnfails;
	qsort(fails, nfails, sizeof(*fails), fail_cmp);
	for (j = 0; j < nfails; j++)
		getcol(j, nfails, &fails[j]->f_fill);
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
	size_t j, newntemps;
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

	newntemps = 0;
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
			temp = getobj(&t, (void ***)&temps,
			    ntemps, &newntemps, &maxtemps, temp_eq, TINCR,
			    sizeof(struct temp));
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

	ntemps = newntemps;
	qsort(temps, ntemps, sizeof(*temps), temp_cmp);
	for (j = 0; j < ntemps; j++)
		getcol_temp(j, &temps[j]->t_fill);
}
