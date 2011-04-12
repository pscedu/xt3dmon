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

#include "buf.h"
#include "cdefs.h"
#include "ds.h"
#include "dynarray.h"
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

#define PS_ALLOWSPACE (1 << 0)

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

struct datafield {
	char				*df_name;
	int				 df_type;
	union {
		struct buf		 dfu_scalar;
		struct dynarray		 dfu_array;
	}				 df_udata;
#define df_scalar	df_udata.dfu_scalar
#define df_array	df_udata.dfu_array
#define df_mapfields	df_udata.dfu_array
};

#define DFT_SCALAR	(1 << 0)
#define DFT_ARRAY	(1 << 1)
#define DFT_MAP		(1 << 2)

void
datafield_new(int type)
{
	struct datafield *df;

	df = malloc(sizeof(*df));
	df->df_type = type;
	switch (type) {
	case DFT_SCALAR:
		buf_init(&df->df_scalar);
		break;
	case DFT_ARRAY:
		dynarray_init(&df->df_array);
		break;
	case DFT_MAP:
		dynarray_init(&df->df_mapfields);
		break;
	}
	return (df);
}

void
datafield_free(struct datafield *df)
{
	struct dynarray a;
	int i;

	dynarray_init(a);
	dynarray_push(a, df);
	while (dynarray_len(a)) {
		free(df->df_name);

		df = dynarray_getpos(a, 0);
		switch (df->df_type) {
		case DFT_SCALAR:
			buf_free(df->df_scalar);
			break;
		case DFT_ARRAY:
			DYNARRAY_FOREACH(df, i, &df->df_array)
				dynarray_push(a, df);
			dynarray_free(df->df_array);
			break;
		case DFT_MAP:
			DYNARRAY_FOREACH(df, i, &df->df_mapfields)
				dynarray_push(a, df);
			dynarray_free(df->df_mapfields);
			break;
		}
	}
	dynarray_free(a);
}

struct datafield *
datafield_map_getkeyvalue(struct datafield *p, const char *name)
{
	struct datafield *df;
	int n;

	if (p->df_type != DFT_MAP)
		return (NULL);
	DYNARRAY_FOREACH(df, n, p->df_mapfields)
		if (strcmp(buf_get(&df->df_name), name) == 0)
			return (df);
	return (NULL);
}

#define PARSE_ERROR(ds, fmt, ...) warnx("parse error")

struct datafield *parse_datafield(struct datasrc *);
struct datafield *parse_datafield_array(struct datasrc *);
struct datafield *parse_datafield_map(struct datasrc *);

struct datafield *
parse_datafield_scalar(struct datasrc *ds)
{
	struct datafield *df;
	int ch;

	df = datafield_new(DFT_SCALAR);
	do {
		ch = us_getchar(ds->ds_us);
		switch (ch) {
		case '\\':
			if (esc)
				buf_append(&df->df_scalar, ch);
			ch = 0;
			break;
		case '"':
			if (esc) {
				buf_append(&df->df_scalar, ch);
				/* hack for loop */
				ch = 0;
			}
			break;
		case '\0':
			PARSE_ERROR(ds, "");
			free(df);
			return (NULL);
		default:
			buf_append(&df->df_scalar, ch);
			break;
		}
		esc = (ch == '\\');
	} while (ch != '"')
	buf_nul(&df->df_scalar);

	ch = us_getchar(ds->ds_us);
	while (isspace(ch))
		ch = us_getchar(ds->ds_us);
	us_ungetchar(ds->ds_us, ch);
	return (df);
}

struct datafield *
parse_datafield_array(struct datasrc *ds)
{
	struct datafield *df, *c;
	int ch;

	df = datafield_new(DFT_ARRAY);

	ch = us_getchar(ds->ds_us);
	if (ch != '[') {
		PARSE_ERROR(ds, "");
		return (NULL);
	}
	do {
		c = NULL;
		while (isspace(ch))
			ch = us_getchar(ds->ds_us);
		ch = us_getchar(ds->ds_us);
		switch (ch) {
		case '"':
			c = parse_datafield_scalar(ds);
			break;
		case '[':
			c = parse_datafield_array(ds);
			break;
		case '{':
			c = parse_datafield_map(ds);
			break;
		case ',':
		case ']':
			break;
		default:
			PARSE_ERROR(ds, "");
			return (NULL);
		}
		if (c)
			dynarray_push(df->df_array, c);
	} while (ch != ']');

	ch = us_getchar(ds->ds_us);
	while (isspace(ch))
		ch = us_getchar(ds->ds_us);
	us_ungetchar(ds->ds_us, ch);
}

struct datafield *
parse_datafield_map(struct datasrc *ds)
{
	struct datafield *df, *c;
	int ch;

	df = datafield_new(DFT_MAP);

	ch = us_getchar(ds->ds_us);
	if (ch != '{') {
		PARSE_ERROR(ds, "");
		return (NULL);
	}
	do {
		ch = us_getchar(ds->ds_us);
		while (isspace(ch))
			ch = us_getchar(ds->ds_us);
		switch (ch) {
		case '"':
			c = parse_datafield(ds);
			if (c == NULL) {
				PARSE_ERROR(ds, "");
				return (NULL);
			}

			dynarray_push(df->df_mapfields, c);
			ch = us_getchar(ds->ds_us);
			switch (ch) {
			case '}':
			case ',':
				break;
			default:
				PARSE_ERROR(ds, "");
				return (NULL);
			}
			break;
		case '}':
			break;
		default:
			PARSE_ERROR(ds, "");
			return (NULL);
		}
	} while (ch != '}');

	ch = us_getchar(ds->ds_us);
	while (isspace(ch))
		ch = us_getchar(ds->ds_us);
	us_ungetchar(ds->ds_us, ch);
	return (df);
}

struct datafield *
parse_datafield(struct datasrc *ds)
{
	struct datafield *df = NULL;
	struct buf namebuf;
	int ch, type;

	buf_init(&namebuf);
	ch = us_getchar(ds->ds_us);
	do {
		switch (ch) {
		case '"':
			break;
		case '\0':
			PARSE_ERROR(ds, "");
			return (NULL);
		default:
			buf_append(&namebuf, ch);
			break;
		}
	} while (ch != '"');

	buf_nul(&namebuf);
	ch = us_getchar(ds->ds_us);
	while (isspace(ch))
		ch = us_getchar(ds->ds_us);
	if (ch != ':') {
		PARSE_ERROR(ds, "");
		return (NULL);
	}
	ch = us_getchar(ds->ds_us);
	while (isspace(ch))
		ch = us_getchar(ds->ds_us);

	switch (ch) {
	case '"':
		df = parse_datafield_scalar(ds);
		break;
	case '[':
		df = parse_datafield_array(ds);
		break;
	case '{':
		df = parse_datafield_map(ds);
		break;
	default:
		PARSE_ERROR(ds, "");
		return (NULL);
	}

	df->df_name = buf_get(&namebuf);

	ch = us_getchar(ds->ds_us);
	while (isspace(ch))
		ch = us_getchar(ds->ds_us);
	us_ungetchar(ds->ds_us, ch);
	return (df);
}

/*
	data = {"version":"1.1","result":{
		"jobs":[{"session_id":"174067",...},...],
		"history":[{"session_id":"174004",...},...],
		"queue":[{"Job_Name":"12s-4-4-1",...},...],
		"sysinfo":{"mempercpu":8,"gb_per_memnode":64,"mem":16384}}}
*/
void
parse_data(const struct datasrc *ds)
{
	char buf[BUFSIZ], *s, *field;
	int nid, stat, nstate, enabled, jobid, i;
	struct datafield *data, *p, *j, *df, *df_n, *state;
	struct node *n, **np;
	struct physcoord pc;
	struct job *job;
	size_t k;

	NODE_FOREACH_PHYS(n, np) {
		n->n_flags &= ~NF_VALID;
		n->n_job = NULL;
	}

	READ_STRLIT(ds->ds_us, "data");
	READ_SPACE(ds->ds_us);
	READ_STRLIT(ds->ds_us, "=");
	READ_SPACE(ds->ds_us);
	data = parse_datafield_map(ds);

	p = datafield_map_getkeyvalue(data, "sysinfo");
	if (p && p->dfp_type == DFT_MAP) {
		df = datafield_map_getkeyvalue(p, "nodes");
		if (df && df->df_type == DFT_ARRAY) {
			DYNARRAY_FOREACH(df_n, i, df->df_array) {
				state = datafield_map_getkeyvalue(df_n,
				    "state");
				if (state == NULL ||
				    state->df_type != DFT_SCALAR)
					continue;
				switch (buf_get(state->df_scalar)[0]) {
				case 'b':
					n->n_state = SC_BOOT;
					break;
				case 'x':
					n->n_state = SC_NOTSCHED;
					break;
				case 'a':
					n->n_state = SC_ALLOC;
					break;
				case 'f':
					n->n_state = SC_FREE;
					break;
				}
			}
		}
	}

	p = datafield_map_getkeyvalue(data, "jobs");
	if (p && p->dfp_type == DFT_ARRAY) {
		FOREACH() {
		}
	}

	datafield_free(data);

	while (us_gets(ds->ds_us, buf, sizeof(buf)) != NULL) {
		s = buf;
		while (isspace(*s))
			s++;

		PARSENUM(s, nid, machine.m_nidmax);
		PARSENUM(s, pc.pc_row, NROWS);
		PARSENUM(s, pc.pc_node, NNODES);
		PARSECHAR(s, stat);
		PARSENUM(s, enabled, 2);
		PARSENUM(s, jobid, INT_MAX);

		n = node_for_pc(&pc);
		if (node_nidmap[nid]) {
			warnx("already saw node with nid %d", nid);
			goto bad;
		}
		n->n_nid = nid;
		node_nidmap[nid] = n;

		if (jobid) {
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
		prerror("node", 0, buf, s);
	}
	if (us_sawerror(ds->ds_us))
		warnx("us_gets: %s", us_errstr(ds->ds_us));
	errno = 0;

	for (k = 0; k < job_list.ol_tcur; k++) {
		job = OLE(job_list, k, job);
		col_get_hash(&job->j_oh, job->j_id, &job->j_fill);
	}
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
