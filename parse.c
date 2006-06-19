/* $Id$ */

#include "mon.h"

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "cdefs.h"
#include "ds.h"
#include "fill.h"
#include "flyby.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "state.h"
#include "ustream.h"
#include "yod.h"

#define PS_ALLOWSPACE (1<<0)

struct route	 rt_max;
struct seastar	 ss_max;
struct ivec	 widim;
int		 total_failures;
unsigned long	 vmem;
long		 rmem;

int		 job_ca_cookie;
int		 yod_ca_cookie;

/*
 *	s	points to start of number in a string
 *	v	variable obtaining numeric result
 *	max	largest value (exclusive)
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

#define PARSEDBL(s, v)						\
	do {							\
		char *_p;					\
								\
		while (isspace(*(s)))				\
			(s)++;					\
		_p = (s);					\
		(v) = strtod((s), &_p);				\
								\
		if ((s) == _p || !isspace(*_p))			\
			goto bad;				\
		(s) = _p;					\
		*(s)++ = '\0';					\
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
	if (!isalnum(*s) && !ispunct(*s))
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
 * c9-1c0s7s0
 */
int
parsenid(char *nid, struct physcoord *pc)
{
	char *s, *t;
	long l;

	s = nid;
	while (isspace(*s))
		s++;
	if (*s != 'c')
		return (1);
	t = ++s;
	while (isdigit(*t))
		t++;
	if (*t != '-')
		return (1);
	*t++ = '\0';
	l = strtol(s, NULL, 10);
	if (l < 0 || l >= NCABS)
		return (1);
	pc->pc_cb = l;

	s = t;
	while (isdigit(*++t))
		;
	if (*t != 'c')
		return (1);
	*t++ = '\0';
	l = strtol(s, NULL, 10);
	if (l < 0 || l >= NROWS)
		return (1);
	pc->pc_r = l;

	s = t;
	while (isdigit(*++t))
		;
	if (*t != 's')
		return (1);
	*t++ = '\0';
	l = strtol(s, NULL, 10);
	if (l < 0 || l >= NCAGES)
		return (1);
	pc->pc_cg = l;

	s = t;
	while (isdigit(*++t))
		;
	if (*t != 's')
		return (1);
	*t++ = '\0';
	l = strtol(s, NULL, 10);
	if (l < 0 || l >= NMODS)
		return (1);
	pc->pc_m = l;

	s = t;
	while (isdigit(*++t))
		;
	if (*t != '\0')
		return (1);
	l = strtol(s, NULL, 10);
	if (l < 0 || l >= NNODES)
		return (1);
	pc->pc_n = l;

	return (0);
}

void
prerror(const char *fn, int lineno, const char *bufp, const char *s)
{
	char *t, buf[BUFSIZ];

	strncpy(buf, bufp, sizeof(buf));
	if ((t = strchr(buf, '\n')) != NULL)
		*t = '\0';
	for (t = buf; t != NULL; )
		if ((t = strchr(t, '\r')) != NULL)
			*t++ = '\0';
	warnx("%s:%d: malformed line: %s [%s]", fn, lineno, buf,
	    buf + (s - bufp));
}

/*
 * Example line:
 *	nid	r cb cb m n	x y z	stat	enabled	jobid	temp	yodid	nfails	lustat
 *	1848	0 7  0  1 6	7 0 9	c	1	6036	45	10434	0	c
 */
void
parse_node(const struct datasrc *ds)
{
	int lineno, r, cb, cg, m, n, nid, x, y, z, stat;
	int enabled, jobid, temp, yodid, nfails, lustat;
	char buf[BUFSIZ], *s;
	struct node *node;
	size_t j;

	widim.iv_w = widim.iv_h = widim.iv_d = 0;

	/* Explicitly initialize all nodes. */
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_flags |= NF_HIDE;
						node->n_job = NULL;
						node->n_yod = NULL;
					}

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#' || *s == '\0')
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
		PARSECHAR(s, lustat);

		node = &nodes[r][cb][cg][m][n];
		node->n_nid = nid;
		invmap[nid] = node;
		wimap[x][y][z] = node;
		node->n_wiv.iv_x = x;
		node->n_wiv.iv_y = y;
		node->n_wiv.iv_z = z;

		widim.iv_w = MAX(x, widim.iv_w);
		widim.iv_h = MAX(y, widim.iv_h);
		widim.iv_d = MAX(z, widim.iv_d);

		switch (stat) {
		case 'c': /* compute */
			node->n_state = SC_FREE;

			if (enabled == 0)
				node->n_state = SC_DISABLED;
			break;
		case 'n': /* down */
			node->n_state = SC_DOWN;
			break;
		case 'i': /* service */
			node->n_state = SC_SVC;
			break;
		default:
			warnx("node %d: bad state %c",
			    nid, stat);
			goto bad;
		}

		switch (lustat) {
		case 'c':
			node->n_lustat = LS_CLEAN;
			break;
		case 'd':
			node->n_lustat = LS_DIRTY;
			break;
		case 'r':
			node->n_lustat = LS_RECOVER;
			break;
		default:
			warnx("node %d: bad lustat %c",
			    nid, lustat);
			goto bad;
		}

		node->n_temp = temp ? temp : DV_NODATA;
		node->n_fails = nfails ? nfails : DV_NODATA;

		if (jobid) {
			if (node->n_state == SC_USED)
				node->n_state = SC_FREE;
			node->n_job = obj_get(&jobid, &job_list);
			node->n_job->j_id = jobid;
		}

		if (yodid) {
			node->n_yod = obj_get(&yodid, &yod_list);
			node->n_yod->y_id = yodid;
		}

		node->n_flags &= ~NF_HIDE;
		continue;
bad:
		prerror("node", lineno, buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;

	/*
	 * XXXX: Disgusting special case hack for reel mode:
	 * jobs in reel mode should have the same color
	 * for their duration, so use a separate color
	 * allocation algorithm for them.
	 */
//	if (st.st_opts & OP_REEL &&
//	    flyby_mode == FBM_PLAY) {
		for (j = 0; j < job_list.ol_tcur; j++)
			if ((job_list.ol_jobs[j]->j_oh.oh_flags & OHF_OLD) == 0)
				col_get_intv(&job_ca_cookie,
				    &job_list.ol_jobs[j]->j_fill);
		for (j = 0; j < yod_list.ol_tcur; j++)
			if ((yod_list.ol_yods[j]->y_oh.oh_flags & OHF_OLD) == 0)
				col_get_intv(&yod_ca_cookie,
				    &yod_list.ol_yods[j]->y_fill);
//	} else {
//		qsort(job_list.ol_jobs, job_list.ol_tcur,
//		    sizeof(struct job *), job_cmp);
//		for (j = 0; j < job_list.ol_tcur; j++)
//			col_get(job_list.ol_jobs[j]->j_oh.oh_flags & OHF_OLD,
//			    j, job_list.ol_tcur, &job_list.ol_jobs[j]->j_fill);
//
//		qsort(yod_list.ol_yods, yod_list.ol_tcur,
//		    sizeof(struct yod *), yod_cmp);
//		for (j = 0; j < yod_list.ol_tcur; j++)
//			col_get(yod_list.ol_yods[j]->y_oh.oh_flags & OHF_OLD,
//			    j, yod_list.ol_tcur, &yod_list.ol_yods[j]->y_fill);
//	}

	if (++widim.iv_w != WIDIM_WIDTH ||
	    ++widim.iv_h != WIDIM_HEIGHT ||
	    ++widim.iv_d != WIDIM_DEPTH) {
		warnx("wired cluster dimensions have changed (%d,%d,%d)",
		    widim.iv_w, widim.iv_y, widim.iv_z);
		widim.iv_w = WIDIM_WIDTH;
		widim.iv_h = WIDIM_HEIGHT;
		widim.iv_d = WIDIM_DEPTH;
	}
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
		if (*s == '#' || *s == '\0')
			continue;

		PARSENUM(s, y_fake.y_id, INT_MAX);

		if ((y = yod_findbyid(y_fake.y_id, NULL)) == NULL)
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
		prerror("yod", lineno, buf, s);
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
		if (*s == '#' || *s == '\0')
			continue;

		PARSENUM(s, j_fake.j_id, INT_MAX);

		if ((j = job_findbyid(j_fake.j_id, NULL)) == NULL)
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
		if (parsestr(&s, j_fake.j_name, sizeof(j_fake.j_name),
		    PS_ALLOWSPACE))
			goto bad;

		*j = j_fake;
		continue;
bad:
		prerror("job", lineno, buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;
}

/*
 * nid        port recover fatal router
 * c9-1c0s7s0 0    0       1     1
 */
void
parse_rt(const struct datasrc *ds)
{
	int port, lineno, recover, fatal, router;
	char *s, buf[BUFSIZ], nid[BUFSIZ];
	struct physcoord pc;
	struct node *n;
	struct ivec iv;
	int j, k;

	NODE_FOREACH(n, &iv)
		if (n)
			for (j = 0; j < NRP; j++)
				for (k = 0; k < NRT; k++)
					n->n_route.rt_err[j][k] = DV_NODATA;
	memset(&rt_max, 0, sizeof(rt_max));

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#' || *s == '\0')
			continue;

		if (parsestr(&s, nid, sizeof(nid), 0) ||
		    parsenid(nid, &pc))
			goto bad;
		n = &nodes[pc.pc_r][pc.pc_cb][pc.pc_cg][pc.pc_m][pc.pc_n];

		PARSENUM(s, port, 7);
		PARSENUM(s, recover, INT_MAX);
		PARSENUM(s, fatal, INT_MAX);
		PARSENUM(s, router, INT_MAX);

		rt_max.rt_err[port][RT_RECOVER] += recover;
		rt_max.rt_err[port][RT_FATAL]   += fatal;
		rt_max.rt_err[port][RT_ROUTER]  += router;

		n->n_route.rt_err[port][RT_RECOVER] = recover;
		n->n_route.rt_err[port][RT_FATAL]   = fatal;
		n->n_route.rt_err[port][RT_ROUTER]  = router;
		continue;
bad:
		prerror("rt", lineno, buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;
}

/*
 * nid       	nblk		nflt		npkt
 * c9-1c0s7s0	0 0 0 0		0 0 0 0		0 0 0 0
 */
void
parse_ss(const struct datasrc *ds)
{
	char *s, buf[BUFSIZ], nid[BUFSIZ];
	int lineno, vc, cnt;
	struct physcoord pc;
	struct seastar ss;
	struct node *n;
	struct ivec iv;

	NODE_FOREACH(n, &iv)
		if (n)
			memset(&n->n_sstar, 0, sizeof(n->n_sstar));
	memset(&ss_max, 0, sizeof(ss_max));

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#')
			continue;

		if (parsestr(&s, nid, sizeof(nid), 0) ||
		    parsenid(nid, &pc))
			goto bad;
		n = &nodes[pc.pc_r][pc.pc_cb][pc.pc_cg][pc.pc_m][pc.pc_n];

		for (vc = 0; vc < NVC; vc++)
			PARSEDBL(s, ss.ss_cnt[SSCNT_NBLK][vc]);
		for (vc = 0; vc < NVC; vc++)
			PARSEDBL(s, ss.ss_cnt[SSCNT_NFLT][vc]);
		for (vc = 0; vc < NVC; vc++)
			PARSEDBL(s, ss.ss_cnt[SSCNT_NPKT][vc]);

		for (vc = 0; vc < NVC; vc++)
			for (cnt = 0; cnt < NSSCNT; cnt++)
				if (ss.ss_cnt[cnt][vc] > ss_max.ss_cnt[cnt][vc])
					ss_max.ss_cnt[cnt][vc] = ss.ss_cnt[cnt][vc];

		memcpy(&n->n_sstar, &ss, sizeof(n->n_sstar));
		continue;
bad:
		warnx("rt:%d: malformed line [%s] [%s]", lineno, buf, s);
	}
	if (us_error(ds->ds_us))
		warn("us_gets");
	errno = 0;
}
