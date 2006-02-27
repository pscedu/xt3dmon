/* $Id$ */

#include "compat.h"

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "mon.h"
#include "ustream.h"

struct ivec	 widim;
int		 total_failures;
unsigned long	 vmem;
long		 rmem;

/*
 *	s	points to start of number in a string
 *	v	variable obtaining numeric result
 */
#define PARSENUM(s, v, max)					\
	do {							\
		char *_p;					\
		long _l;					\
								\
		while (isspace(*(s)))				\
			(s)++;					\
		_p = (s);					\
		while (isdigit(*(s)))				\
			(s)++;					\
		if ((s) == _p || !isspace(*(s)))		\
			goto bad;				\
		*(s)++ = '\0';					\
		if ((_l = strtoul(_p, NULL, 10)) < 0 ||		\
		    _l >= (max))				\
			goto bad;				\
		(v) = (int)_l;					\
	} while (0)

#define PARSECHAR(s, ch)					\
	do {							\
		while (isspace(*(s)))				\
			(s)++;					\
		if (!isalpha(*(s)))				\
			goto bad;				\
		(ch) = *(s);					\
		if (!isspace(*++(s)))				\
			goto bad;				\
	} while (0)

/*
 * Example line:
 *	nid	r cb cb m n	x y z	stat	enabled	jobid	temp	yodid	nfails
 *	1848	0 7  0  1 6	7 0 9	c	1	6036	45	10434	0
 */
void
parse_node(const struct datasrc *ds)
{
	int lineno, r, cb, cg, m, n, nid, x, y, z, stat;
	int enabled, jobid, temp, yodid, nfails;
	char buf[BUFSIZ], *s;
	struct node *node;
	size_t j;

	widim.iv_w = widim.iv_h = widim.iv_d = 0;

	/* Explicitly initialize all nodes. */
//	IVEC_FOREACH()
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_flags |= NF_HIDE;
					}

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#')
			continue;

		PARSENUM(s, nid, NID_MAX);
		PARSENUM(s, r, NROWS);
		PARSENUM(s, cb, NCABS);
		PARSENUM(s, cg, NCAGES);
		PARSENUM(s, m, NMODS);
		PARSENUM(s, n, NNODES);
		PARSENUM(s, x, WIDIM_WIDTH);
		PARSENUM(s, y, WIDIM_HEIGHT);
		PARSENUM(s, z, WIDIM_DEPTH);
		PARSECHAR(s, stat);
		PARSENUM(s, enabled, 2);
		PARSENUM(s, jobid, INT_MAX);
		PARSENUM(s, temp, INT_MAX);
		PARSENUM(s, yodid, INT_MAX);
		PARSENUM(s, nfails, INT_MAX);

		node = &nodes[r][cb][cg][m][n];
		node->n_nid = nid;
		invmap[nid] = node;
		wimap[x][y][z] = node;
		node->n_wiv.iv_x = x;
		node->n_wiv.iv_y = y;
		node->n_wiv.iv_z = z;

		if (x > widim.iv_w)
			widim.iv_w = x;
		if (y > widim.iv_h)
			widim.iv_h = y;
		if (z > widim.iv_d)
			widim.iv_d = z;

		node->n_state = SC_FREE;

		if (enabled == 0)
			node->n_state = SC_DISABLED;

		node->n_temp = temp ? temp : DV_NODATA;
		node->n_fails = nfails ? nfails : DV_NODATA;

		if (jobid) {
			node->n_state = SC_USED;		/* don't set */
			node->n_job = getobj(&jobid, &job_list);
			node->n_job->j_id = jobid;
		} else
			node->n_job = NULL;

		if (yodid) {
			node->n_yod = getobj(&yodid, &yod_list);
			node->n_yod->y_id = yodid;
		} else
			node->n_yod = NULL;

		switch (stat) {
		case 'c': /* compute */
			break;
		case 'n': /* down */
			node->n_state = SC_DOWN;
			break;
		case 'i': /* service */
			node->n_state = SC_SVC;
			break;
		default:
			goto bad;
		}
		node->n_flags &= ~NF_HIDE;
		continue;
bad:
		warnx("node:%d: malformed line [%s] [%s]", lineno,
		    buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;

	qsort(job_list.ol_jobs, job_list.ol_tcur, sizeof(struct job *), job_cmp);
	for (j = 0; j < job_list.ol_tcur; j++)
		getcol(job_list.ol_jobs[j]->j_oh.oh_flags & OHF_OLD,
		    j, job_list.ol_tcur, &job_list.ol_jobs[j]->j_fill);

	qsort(yod_list.ol_yods, yod_list.ol_tcur, sizeof(struct yod *), yod_cmp);
	for (j = 0; j < yod_list.ol_tcur; j++)
		getcol(yod_list.ol_yods[j]->y_oh.oh_flags & OHF_OLD,
		    j, yod_list.ol_tcur, &yod_list.ol_yods[j]->y_fill);

	if (++widim.iv_w != WIDIM_WIDTH ||
	    ++widim.iv_h != WIDIM_HEIGHT ||
	    ++widim.iv_d != WIDIM_DEPTH)
		errx(1, "wired cluster dimensions have changed");
}

void
parse_mem(const struct datasrc *ds)
{
	FILE *fp;

	fp = ds->ds_us->us_fp; /* XXXXXXX use us_scanf. */
	fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
	    "%*u %*u %*d %*d %*d %*d %*d %*d %*u %lu %ld", &vmem, &rmem);
	if (ferror(fp))
		warn("fscanf");
	errno = 0;
}

/*
 *	yodid	partid	ncpus	cmd
 *	12253	6864	128	yod /usr/
 */
void
parse_yod(const struct datasrc *ds)
{
	int lineno, yodid, partid, ncpus;
	char *s, *p, buf[BUFSIZ];
	struct yod *y;

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#')
			continue;

		PARSENUM(s, yodid, INT_MAX);
		PARSENUM(s, partid, INT_MAX);
		PARSENUM(s, ncpus, INT_MAX);

		while (isspace(*s))
			s++;
		if ((p = strchr(s, '\n')) != NULL)
			*p = '\0';

		if ((y = yod_findbyid(yodid)) != NULL) {
			y->y_partid = partid;
			y->y_ncpus = ncpus;
			strncpy(y->y_cmd, s, sizeof(y->y_cmd) - 1);
			y->y_cmd[sizeof(y->y_cmd) - 1] = '\0';
		}

		continue;
bad:
		warnx("node:%d: malformed line [%s] [%s]", lineno,
		    buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;
}

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
parse_job(const struct datasrc *ds)
{
	char state, *t, *s, *q, buf[BUFSIZ], *next;
	struct job j_fake, *job;
	int jobid;

	s = NULL; /* gcc */
	jobid = 0;
	state = '\0';
	for (;;) {
		next = us_gets(ds->ds_us, buf, sizeof(buf));
		if (next == NULL && us_error(ds->ds_us)) {
			warn("%s", _PATH_JOB);
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

			if (us_eof(ds->ds_us))
				break;
			else {
				s += strlen(q);
				jobid = strtoul(s, NULL, 10);
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
			if ((q = strchr(s, '@')) != NULL)
				*q = '\0';
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
			j_fake.j_ncpus = strtoul(s, NULL, 10);
		}

		q = "Resource_List.walltime = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			if ((t = strchr(s, ':')) != NULL)
				*t++ = '\0';
			j_fake.j_tmdur = 60 * strtoul(s, NULL, 10);
			j_fake.j_tmdur += strtoul(t, NULL, 10);
		}

		q = "resources_used.walltime = ";
		if ((s = strstr(buf, q)) != NULL) {
			s += strlen(q);
			if ((t = strchr(s, ':')) != NULL)
				*t++ = '\0';
			j_fake.j_tmuse = 60 * strtoul(s, NULL, 10);
			j_fake.j_tmuse += strtoul(t, NULL, 10);
		}
	}
}
