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

#define PS_ALLOWSPACE (1<<0)

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
		if ((_l = strtol(_p, NULL, 10)) < 0 ||		\
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

int
parsestr(char **sp, char *buf, size_t siz, int flags)
{
	char *s, *t;
	int error;

	error = 1;
	s = *sp;
	while (isspace(*s))
		s++;
	if (!isalpha(*s) && !ispunct(*s))
		goto bad;
	for (t = s; *t != '\0'; t++) {
		/* Find terminating whitespace. */
		for (; *t != '\0' && !isspace(*t); t++)
			;
		if (*t == '\0')
			goto bad;

		/*
		 * Continue to look for EOL if space allowed and
		 * EOL not found, else quit since normal space found.
		 */
		if ((flags & PS_ALLOWSPACE) == 0 || *t == '\n')
			break;
	}
	*t++ = '\0';
	strncpy(buf, s, siz - 1);
	buf[siz - 1] = '\0';
	s = t;

	error = 0;
bad:
	*sp = s;
	return (error);
}

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
		warnx("node:%d: malformed line [%s] [%s]\n", lineno, buf, s);
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
		errx(1, "wired cluster dimensions have changed (%d,%d,%d)",
		    widim.iv_w, widim.iv_y, widim.iv_z);
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
	struct yod *y, y_fake;
	char *s, buf[BUFSIZ];
	int lineno;

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#')
			continue;

		PARSENUM(s, y_fake.y_id, INT_MAX);

		if ((y = yod_findbyid(y_fake.y_id)) == NULL)
			continue;
		else
			y_fake = *y;

		PARSENUM(s, y_fake.y_partid, INT_MAX);
		PARSENUM(s, y_fake.y_ncpus, INT_MAX);
		if (parsestr(&s, y_fake.y_cmd, sizeof(y_fake.y_cmd),
		    PS_ALLOWSPACE))
			goto bad;

		*y = y_fake;
		continue;
bad:
		warnx("yod:%d: malformed line [%s] [%s]", lineno, buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;
}

/*
 * id      owner   tmdur   tmuse   mem     ncpus  queue name
 * 20161   devivo  270     75      37868   320	  batch cmdname
 */
void
parse_job(const struct datasrc *ds)
{
	struct job j_fake, *j;
	char *s, buf[BUFSIZ];
	int lineno;

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#')
			continue;

		PARSENUM(s, j_fake.j_id, INT_MAX);

		if ((j = job_findbyid(j_fake.j_id)) == NULL)
			continue;
		else
			j_fake = *j;

		if (parsestr(&s, j_fake.j_owner, sizeof(j_fake.j_owner), 0))
			goto bad;
		PARSENUM(s, j_fake.j_tmdur, INT_MAX);
		PARSENUM(s, j_fake.j_tmuse, INT_MAX);
		PARSENUM(s, j_fake.j_mem, INT_MAX);
		PARSENUM(s, j_fake.j_ncpus, INT_MAX);
		if (parsestr(&s, j_fake.j_queue, sizeof(j_fake.j_queue), 0))
			goto bad;
		if (parsestr(&s, j_fake.j_jname, sizeof(j_fake.j_jname),
		    PS_ALLOWSPACE))
			goto bad;

		*j = j_fake;
		continue;
bad:
		warnx("job:%d: malformed line [%s] [%s]", lineno, buf, s);
	}
}
