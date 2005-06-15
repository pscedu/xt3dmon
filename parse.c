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
void		*getobj(void *arg, void ***data, size_t *cursiz,
    size_t *maxsiz, cmpf_t eq, int inc, size_t);

size_t		 njobs, maxjobs;
size_t		 nfails, maxfails;
size_t		 ntemps, maxtemps;

struct fail	**fails;
struct job	**jobs;
struct temp	**temps;

int		 total_failures;

struct fail fail_notfound = {
	0, { 0.33f, 0.66f, 0.99f, 1.00f, 0 }, "0"
};

struct temp temp_notfound = {
	0, { 0.00f, 0.00f, 0.00f, 1.00f, 0 }, "?"
};

int job_eq(void *elem, void *arg)
{
	return (((struct job *)elem)->j_id == *(int *)arg);
}

int temp_eq(void *elem, void *arg)
{
	return (((struct temp *)elem)->t_cel == *(int *)arg);
}

int fail_eq(void *elem, void *arg)
{
	return (((struct fail *)elem)->f_fails == *(int *)arg);
}

void *
getobj(void *arg, void ***data, size_t *cursiz, size_t *maxsiz,
    cmpf_t eq, int inc, size_t objlen)
{
	void **jj, *j = NULL;
	size_t n, newmax;

	if (*data != NULL)
		for (n = 0, jj = *data; n < *cursiz; jj++, n++)
			if (eq(*jj, arg))
				return (*jj);
	/* Not found; add. */
	if (*cursiz + 1 >= *maxsiz) {
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
	return ((*data)[(*cursiz)++]);
}

void
getcol(int n, size_t total, struct fill *fillp)
{
	double div;

	if (total == 1)
		div = 0.0;
	else
		div = ((double)n) / ((double)(total - 1));

	fillp->f_r = cos(div);
	fillp->f_g = sin(div) * sin(div);
	fillp->f_b = fabs(tan(div + PI * 3/4));
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

	logical_width = 0;
	logical_height = 0;
	logical_depth = 0;

	/* Explicitly initialize all nodes. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_state = JST_UNACC;
						node->n_fillp = &jstates[JST_UNACC].js_fill;
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
		node->n_logv.v_x = x * LOGICAL_NODE_SPACING_FACTOR;
		node->n_logv.v_y = y * LOGICAL_NODE_SPACING_FACTOR;
		node->n_logv.v_z = z * LOGICAL_NODE_SPACING_FACTOR;

		if (node->n_logv.v_x > logical_width)
			logical_width = node->n_logv.v_x;
		if (node->n_logv.v_y > logical_height)
			logical_height = node->n_logv.v_y;
		if (node->n_logv.v_z > logical_depth)
			logical_depth = node->n_logv.v_z;

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
	struct node *node;
	struct job *job;
	FILE *fp;
	size_t j;
	long l;

	/* XXXXXX - reset fillp on all nodes. */

	njobs = 0;
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
			job = getobj(&jobid, (void ***)&jobs,
			    &njobs, &maxjobs, job_eq, JINCR,
			    sizeof(*job));
			job->j_id = jobid;
			job->j_name = "testjob";
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
	int nid, lineno, r, cb, cg, m, n;
	size_t j, newmax, nofails;
	char *p, *s, buf[BUFSIZ];
	struct node *node;
	struct fail *fail;
	FILE *fp;
	long l;

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
	if ((fp = fopen(_PATH_FAILMAP, "r")) == NULL)
		warn("%s", _PATH_FAILMAP);
	else {
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
			fail = getobj(&nofails, (void ***)&fails, &nfails,
			    &maxfails, fail_eq, FINCR, sizeof(struct fail));
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
	}
	for (j = 0; j < maxfails; j++)
		getcol(j, maxfails, &fails[j]->f_fill);
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

	if ((fp = fopen(_PATH_TEMPMAP, "r")) == NULL)
		warn("%s", _PATH_TEMPMAP);
	else {
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
				    &ntemps, &maxtemps, temp_eq, TINCR,
				    sizeof(struct temp));
				temp->t_cel = t;
				free(temp->t_name);
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
	}
	for (j = 0; j < maxtemps; j++)
		getcol(j, maxtemps, &temps[j]->t_fill);
}
