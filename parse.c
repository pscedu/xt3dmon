/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

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
#include "env.h"
#include "fill.h"
#include "flyby.h"
#include "job.h"
#include "mach.h"
#include "node.h"
#include "nodeclass.h"
#include "parse.h"
#include "state.h"
#include "ustream.h"

#define PS_ALLOWSPACE (1<<0)

struct route	 rt_max;
struct route	 rt_zero;

int		 job_ca_cookie;

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
 *	nid	r cb cb m n	x y z	stat	enabled	jobid	temp	yodid	UNUSED	lustat
 *	1848	0 7  0  1 6	7 0 9	c	1	6036	45	10434	0	c
 */
void
parse_node(const struct datasrc *ds)
{
	int enabled, jobid, temp, nfails, lustat;
	int lineno, nid, x, y, z, stat, nstate;
	char buf[BUFSIZ], *s, *field;
	struct node *n, **np;
	struct physcoord pc;
	struct ivec twidim;
	struct job *job;
	size_t j;

	NODE_FOREACH_WI(n, np) {
		n->n_flags &= ~NF_VALID;
		n->n_job = NULL;
	}

	ivec_set(&twidim, 0, 0, 0);
	/* XXX overflow */
	memset(node_nidmap, 0, machine.m_nidmax * sizeof(*node_nidmap));
	memset(node_wimap, 0, node_wimap_len * sizeof(*node_wimap));
	mach_drain = 0;

	lineno = 0;
	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#' || *s == '\0')
			continue;

		if (*s == '@') {
			s++;

			field = "drain";
			if (strncmp(s, field, strlen(field)) == 0) {
				s += strlen(field);
				if (!isspace(*s))
					goto bad;
				PARSENUM(s, mach_drain, INT_MAX); /* XXX TIME_T_MAX */
				continue;
			}

			goto bad;
		}

		PARSENUM(s, nid, machine.m_nidmax);
		PARSENUM(s, pc.pc_r, NROWS);
		PARSENUM(s, pc.pc_cb, NCABS);
		PARSENUM(s, pc.pc_cg, NCAGES);
		PARSENUM(s, pc.pc_m, NMODS);
		PARSENUM(s, pc.pc_n, NNODES);
		PARSENUM(s, x, widim.iv_w);
		PARSENUM(s, y, widim.iv_h);
		PARSENUM(s, z, widim.iv_d);
		PARSECHAR(s, stat);
		PARSENUM(s, enabled, 2);
		PARSENUM(s, jobid, INT_MAX);
		PARSENUM(s, temp, INT_MAX);
		PARSENUM(s, nfails, INT_MAX);
		PARSECHAR(s, lustat);

		switch (stat) {
		case 'c': /* compute */
			nstate = SC_FREE;

			if (enabled == 0)
				nstate = SC_DISABLED;
			break;
		case 'n': /* down */
			nstate = SC_DOWN;
			break;
		case 'i': /* service */
			nstate = SC_SVC;
			break;
		case 'd': /* admindown */
			nstate = SC_ADMDOWN;
			break;
		case 'r': /* route */
			nstate = SC_ROUTE;
			break;
		case 's': /* suspect */
			nstate = SC_SUSPECT;
			break;
		case 'u': /* unavail */
			nstate = SC_UNAVAIL;
			break;
		default:
			warnx("node %d: bad state %c", nid, stat);
			goto bad;
		}

		n = node_for_pc(&pc);
		if (node_nidmap[nid]) {
			warnx("already saw node with nid %d", nid);
			goto bad;
		}
		if (NODE_WIMAP(x, y, z)) {
			warnx("already saw node wiv %d:%d:%d", x, y, z);
			goto bad;
		}
		n->n_nid = nid;
		node_nidmap[nid] = n;
		NODE_WIMAP(x, y, z) = n;
		n->n_wiv.iv_x = x;
		n->n_wiv.iv_y = y;
		n->n_wiv.iv_z = z;
		n->n_state = nstate;

		twidim.iv_w = MAX(x, twidim.iv_w);
		twidim.iv_h = MAX(y, twidim.iv_h);
		twidim.iv_d = MAX(z, twidim.iv_d);

		switch (lustat) {
		case 'c':
			n->n_lustat = LS_CLEAN;
			break;
		case 'd':
			n->n_lustat = LS_DIRTY;
			break;
		case 'r':
			n->n_lustat = LS_RECOVER;
			break;
		default:
			warnx("node %d: invalid lustat %c",
			    nid, lustat);
			goto bad;
		}

		n->n_temp = temp ? temp : DV_NODATA;

		if (jobid) {
			if (n->n_state == SC_FREE)
				n->n_state = SC_USED;
			n->n_job = obj_get(&jobid, &job_list);
			n->n_job->j_id = jobid;
			if (strcmp(n->n_job->j_name, "") == 0)
				snprintf(n->n_job->j_name,
				    sizeof(n->n_job->j_name),
				    "job %d", jobid);
		}

		n->n_flags |= NF_VALID;
		continue;
bad:
		prerror("node", lineno, buf, s);
	}
	if (us_sawerror(ds->ds_us))
		warnx("us_gets: %s", us_errstr(ds->ds_us));
	errno = 0;

	for (j = 0; j < job_list.ol_tcur; j++) {
		job = OLE(job_list, j, job);
		col_get_hash(&job->j_oh, job->j_id, &job->j_fill);
	}

	if (++twidim.iv_w != widim.iv_w ||
	    ++twidim.iv_h != widim.iv_h ||
	    ++twidim.iv_d != widim.iv_d)
		errx(1, "wired cluster dimensions have changed (%d,%d,%d)",
		    twidim.iv_w, twidim.iv_y, twidim.iv_z);
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

		if (strcmp(j_fake.j_name, DV_NOAUTH) == 0)
			snprintf(j_fake.j_name, sizeof(j_fake.j_name),
			    "job %d", j_fake.j_id);

		*j = j_fake;
		continue;
bad:
		prerror("job", lineno, buf, s);
	}
	if (us_sawerror(ds->ds_us))
		warnx("us_gets: %s", us_errstr(ds->ds_us));
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
	struct node *n, **np;
	struct physcoord pc;

	NODE_FOREACH_WI(n, np)
		memset(&n->n_route, 0, sizeof(n->n_route));
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
		n = node_for_pc(&pc);

		PARSENUM(s, port, 7);
		PARSENUM(s, recover, INT_MAX);
		PARSENUM(s, fatal, INT_MAX);
		PARSENUM(s, router, INT_MAX);

/* XXX: max vs. total? */

/*
		rt_max.rt_err[port][RT_RECOVER] = MAX(recover, rt_max.rt_err[port][RT_RECOVER]);
		rt_max.rt_err[port][RT_FATAL]   = MAX(fatal,   rt_max.rt_err[port][RT_FATAL]);
		rt_max.rt_err[port][RT_ROUTER]  = MAX(router,  rt_max.rt_err[port][RT_ROUTER]);
*/
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
	if (us_sawerror(ds->ds_us))
		warnx("us_gets: %s", us_errstr(ds->ds_us));
	errno = 0;
}

void
parse_colors(const char *fn)
{
	int cv, idx, col[3], lineno;
	char *s, *p, buf[BUFSIZ];
	struct color *c;
	FILE *fp;
	long l;

	obj_batch_start(&col_list);

	if ((fp = fopen(fn, "r")) == NULL)
		err(1, "%s", fn);

	idx = 0;
	lineno = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		lineno++;
		s = buf;
		while (isspace(*s))
			s++;
		if (*s == '#' || *s == '\0')
			continue;

		idx = 0;
		do {
			while (*s != '\0' && !isdigit(*s))
				s++;
			if (!isdigit(*s))
				goto bad;
			p = s + 1;

			while (isdigit(*p))
				p++;
			*p++ = '\0';

			l = strtol(s, NULL, 0);
			if (l > 255 || l < 0)
				goto bad;
			col[idx++] = (int)l;

			s = p;
		} while (idx < 3);

		cv = col[0] * 256 * 256 +
		    col[1] * 256 + col[2];
		c = obj_get(&cv, &col_list);
		memcpy(c->c_rgb, col, sizeof(c->c_rgb));
		continue;
bad:
		warnx("col:%d: malformed line [%s] [%s]", lineno, buf, s);
	}
	if (ferror(fp))
		warn("%s", fn);
	fclose(fp);

	obj_batch_end(&col_list);
}
