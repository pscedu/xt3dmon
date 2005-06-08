/* $Id$ */

#include <sys/param.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mon.h"

#define MAXFAILS INT_MAX

struct job		*getjob(int);
void			 getcol(int, struct job *);

int			 logids[] = { 0, 8 };
size_t			 maxjobs;
int			 total_failures;

/*
 * Example line:
 *	c0-1c0s1 3 7 c
 *
 * Broken up:
 *	cb r cg  m	n nid  state
 *	c0-1 c0 s1	3 7    c
 */
void
parse_physmap(void)
{
	char fn[MAXPATHLEN], buf[BUFSIZ], *p, *s;
	int lineno, r, cb, cg, m, n, nid;
	struct node *node;
	FILE *fp;
	size_t j;
	long l;

	/* Explicitly initialize all nodes. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++)
						nodes[r][cb][cg][m][n].n_state = ST_UNACC;

	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_PHYSMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL) {
			warn("%s", fn);
			continue;
		}
		lineno = 0;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			lineno++;
			p = buf;
			while (isspace(*p))
				p++;
			if (*p == '#')
				continue;
			if (*p++ != 'c')
				goto bad;
			if (!isdigit(*p))
				goto bad;

			/* cb */
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
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NCAGES)
				goto bad;
			cg = (int)l;

			/* m */
			p = s + 1;
			s = p;
			while (isdigit(*++s))
				;
			if (!isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NMODS)
				goto bad;
			m = (int)l;

			/* n */
			while (isspace(*++s))
				;
			if (!isdigit(*s))
				goto bad;
			p = s;
			while (isdigit(*++s))
				;
			if (!isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NNODES)
				goto bad;
			n = (int)l;

			/* nid */
			while (isspace(*++s))
				;
			if (!isdigit(*s))
				goto bad;
			p = s;
			while (isdigit(*++s))
				;
			if (!isspace(*s))
				goto bad;
			*s = '\0';
			if ((l = strtoul(p, NULL, 10)) < 0 || l >= NID_MAX)
				goto bad;
			nid = (int)l;

			node = &nodes[r][cb][cg][m][n];
			node->n_nid = nid;
			node->n_logid = logids[j];
			invmap[j][nid] = node;

			/* state */
			while (isspace(*++s))
				;
			if (!isalpha(*s))
				goto bad;
			switch (*s) {
			case 'c': /* compute */
				node->n_state = ST_FREE;	/* Good enough. */
				break;
			case 'n': /* disabled */
				node->n_state = ST_DISABLED;
				break;
			case 'i': /* I/O */
				node->n_state = ST_IO;
				break;
			default:
				goto bad;
			}
			continue;
bad:
			warnx("%s:%d: malformed line [%s] [%s]", fn, lineno, buf, p);
		}
		if (ferror(fp))
			warn("%s", fn);
		fclose(fp);
		errno = 0;
	}
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
	int jobid, nid, lineno, enabled, bad, checking;
	char fn[MAXPATHLEN], buf[BUFSIZ], *p, *s;
	struct node *node;
	FILE *fp;
	size_t j;
	long l;

	for (j = 0; j < njobs; j++)
		free(jobs[j]);
	njobs = 0;
	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_JOBMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL) {
			warn("%s", fn);
			continue;
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

			node = invmap[j][nid];
			if (node == NULL && enabled) {
				warnx("inconsistency: node %d should be "
				    "disabled in jobmap", nid);
				continue;
			}

			if (enabled == 0)
				node->n_state = ST_DOWN;
			else if (jobid == 0)
				node->n_state = ST_FREE;
			else {
				node->n_state = ST_USED;
				node->n_job = getjob(jobid);
			}
			continue;
bad:
			warn("%s:%d: malformed line", fn, lineno);
		}
		if (ferror(fp))
			warn("%s", fn);
		fclose(fp);
		errno = 0;

		snprintf(fn, sizeof(fn), _PATH_BADMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL)
			/* This is OK. */
			errno = 0;
		else {
			lineno = 0;
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				lineno++;
				p = buf;
				while (isspace(*p))
					p++;
				if (*p == '#')
					continue;
				if (!isdigit(*p))
					goto badbad;

				/* nid */
				s = p;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badbad;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
					goto badbad;
				nid = (int)l;

				/* disabled */
				p = s + 1;
				s = p;
				if (!isdigit(*s))
					goto badbad;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badbad;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
					goto badbad;
				bad = (int)l;

				if (bad)
					/* XXX:  check validity. */
					invmap[j][nid]->n_state = ST_BAD;
				continue;
badbad:
				warnx("%s:%d: malformed line", fn, lineno);
				}
			if (ferror(fp))
				warn("%s", fn);
			fclose(fp);
			errno = 0;
		}

		snprintf(fn, sizeof(fn), _PATH_CHECKMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL)
			/* This is OK. */
			errno = 0;
		else {
			lineno = 0;
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				lineno++;
				p = buf;
				while (isspace(*p))
					p++;
				if (*p == '#')
					continue;
				if (!isdigit(*p))
					goto badcheck;

				/* nid */
				s = p;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badcheck;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > NID_MAX)
					goto badcheck;
				nid = (int)l;

				/* disabled */
				p = s + 1;
				s = p;
				if (!isdigit(*s))
					goto badcheck;
				while (isdigit(*++s))
					;
				if (!isspace(*s))
					goto badcheck;
				*s = '\0';
				if ((l = strtoul(p, NULL, 10)) < 0 || l > INT_MAX)
					goto badcheck;
				checking = (int)l;

				if (checking)
					/* XXX:  check validity. */
					invmap[j][nid]->n_state = ST_CHECK;
				continue;
badcheck:
				warnx("%s:%d: malformed line", fn, lineno);
			}
			if (ferror(fp))
				warn("%s", fn);
			fclose(fp);
			fclose(fp);
			errno = 0;
		}
	}

	for (j = 0; j < njobs; j++)
		getcol(j, jobs[j]);
}

#define JINCR 10

struct job *
getjob(int id)
{
	struct job **jj, *j = NULL;
	size_t n;

	if (jobs != NULL)
		for (n = 0, jj = jobs; n < njobs; jj++, n++)
			if ((*jj)->j_id == id)
				return (*jj);
	if (njobs + 1 >= maxjobs) {
		maxjobs += JINCR;
		if ((jobs = realloc(jobs, sizeof(*jobs) * maxjobs)) == NULL)
			err(1, "realloc");
	}
	if ((j = malloc(sizeof(*j))) == NULL)
		err(1, "malloc");
	memset(j, 0, sizeof(*j));
	j->j_id = id;
	jobs[njobs++] = j;
	return (j);
}

void
getcol(int n, struct job *j)
{
	double div;

	if (njobs == 1)
		div = 0.0;
	else
		div = ((double)n) / ((double)(njobs - 1));

	j->j_r = cos(div);
	j->j_g = sin(div) * sin(div);
	j->j_b = fabs(tan(div + PI * 3/4));
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
	int nid, lineno, fails, r, cb, cg, m, n;
	char *p, *s, fn[PATH_MAX], buf[BUFSIZ];
	FILE *fp;
	size_t j;
	long l;

	/*
	 * Because entries with zero failures are not listed,
	 * we must go through and reset all entries.
	 */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++)
						nodes[r][cb][cg][m][n].n_fails = 0;

	total_failures = 0;
	for (j = 0; j < NLOGIDS; j++) {
		snprintf(fn, sizeof(fn), _PATH_FAILMAP, logids[j]);
		if ((fp = fopen(fn, "r")) == NULL) {
			warn("%s", fn);
			continue;
		}
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
			if ((l = strtol(p, NULL, 10)) < 0 || l >= MAXFAILS)
				goto bad;
			fails = (int)l;

			p = s + 1;
			while (isspace(*p))
				p++;
			for (s = p; isdigit(*s); s++)
				;
			if (s == p || *s != '\n')
				goto bad;
			*s = '\0';
			if ((l = strtol(p, NULL, 10)) < 0 || l >= NID_MAX)
				goto bad;
			nid = (int)l;

			invmap[j][nid]->n_fails = fails;
			total_failures += fails;
			continue;
bad:
			warnx("%s:%d: malformed line", fn, lineno);
		}
		if (ferror(fp))
			warn("%s", fn);
		fclose(fp);
		errno = 0;
	}

}
