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

int
skip_data_strlit(struct datasrc *ds, const char *str)
{
	const char *p;
	int c;

	for (p = str; *p; p++) {
		c = us_getc(ds->ds_us);
		if (c != *p)
			return (-1);
	}
	return (0);
}

void
skip_data_space(struct datasrc *ds)
{
	int c;

	do
		c = us_getc(ds->ds_us);
	while (isspace(c));
	us_ungetc(ds->ds_us, c);
}

enum datafield_type {
	DFT_SCALAR,
	DFT_ARRAY,
	DFT_MAP
};

struct datafield {
	char				*df_name;
	enum datafield_type		 df_type;
	union {
		struct buf		 dfu_scalar;
		struct psc_dynarray	 dfu_array;
	}				 df_udata;
#define df_scalar	df_udata.dfu_scalar
#define df_array	df_udata.dfu_array
#define df_mapfields	df_udata.dfu_array
};

struct datafield *
datafield_new(enum datafield_type type)
{
	struct datafield *df;

	df = malloc(sizeof(*df));
	df->df_name = NULL;
	df->df_type = type;
	switch (type) {
	case DFT_SCALAR:
		buf_init(&df->df_scalar);
		break;
	case DFT_ARRAY:
		psc_dynarray_init(&df->df_array);
		break;
	case DFT_MAP:
		psc_dynarray_init(&df->df_mapfields);
		break;
	}
	return (df);
}

void
datafield_free(struct datafield *df)
{
	struct datafield *ch;
	struct psc_dynarray a;
	int i;

	psc_dynarray_init(&a);
	psc_dynarray_push(&a, df);
	while (psc_dynarray_len(&a)) {
		df = psc_dynarray_getpos(&a, 0);
		psc_dynarray_remove(&a, 0);
		switch (df->df_type) {
		case DFT_SCALAR:
			buf_free(&df->df_scalar);
			break;
		case DFT_ARRAY:
			DYNARRAY_FOREACH(ch, i, &df->df_array)
				psc_dynarray_push(&a, ch);
			psc_dynarray_free(&df->df_array);
			break;
		case DFT_MAP:
			DYNARRAY_FOREACH(ch, i, &df->df_mapfields)
				psc_dynarray_push(&a, ch);
			psc_dynarray_free(&df->df_mapfields);
			break;
		}
		free(df->df_name);
		free(df);
	}
	psc_dynarray_free(&a);
}

struct datafield *
datafield_map_getkeyvalue(struct datafield *p, const char *name)
{
	struct datafield *ch;
	int n;

	if (p->df_type != DFT_MAP)
		return (NULL);
	DYNARRAY_FOREACH(ch, n, &p->df_mapfields)
		if (strcmp(ch->df_name, name) == 0)
			return (ch);
	return (NULL);
}

const char *
datafield_map_getscalar(struct datafield *p, const char *name)
{
	struct datafield *df;

	df = datafield_map_getkeyvalue(p, name);
	return (df && df->df_type == DFT_SCALAR ?
	    buf_get(&df->df_scalar) : NULL);
}


#define PARSE_ERROR(ds, fmt, ...)					\
	warnx("parse error (line %d): " fmt, __LINE__, ##__VA_ARGS__) //prerror("node", 0, buf, s);

struct datafield *parse_datafield(struct datasrc *);
struct datafield *parse_datafield_mapentry(struct datasrc *);
struct datafield *parse_datafield_array(struct datasrc *);
struct datafield *parse_datafield_map(struct datasrc *);

struct datafield *
parse_datafield_scalar(struct datasrc *ds)
{
	struct datafield *df;
	int c, esc = 0;

	c = us_getc(ds->ds_us);
	if (c != '"' && !isdigit(c)) {
		PARSE_ERROR(ds, "");
		return (NULL);
	}
	df = datafield_new(DFT_SCALAR);
	if (isdigit(c)) {
		us_ungetc(ds->ds_us, c);
		do {
			c = us_getc(ds->ds_us);
			if (isdigit(c))
				buf_append(&df->df_scalar, c);
		} while (isdigit(c));
		us_ungetc(ds->ds_us, c);
	} else
		do {
			c = us_getc(ds->ds_us);
			switch (c) {
			case '\\':
				if (esc)
					buf_append(&df->df_scalar, c);
				c = 0;
				break;
			case '"':
				if (esc) {
					buf_append(&df->df_scalar, c);
					/* hack for loop */
					c = 0;
				}
				break;
			case '\0':
				PARSE_ERROR(ds, "");
				datafield_free(df);
				return (NULL);
			default:
				buf_append(&df->df_scalar, c);
				break;
			}
			esc = (c == '\\');
		} while (c != '"');
	buf_nul(&df->df_scalar);
	skip_data_space(ds);
	return (df);
}

struct datafield *
parse_datafield_array(struct datasrc *ds)
{
	struct datafield *df, *ch;
	int c;

	c = us_getc(ds->ds_us);
	if (c != '[') {
		PARSE_ERROR(ds, "");
		return (NULL);
	}
	df = datafield_new(DFT_ARRAY);
	do {
		skip_data_space(ds);
		c = us_getc(ds->ds_us);
		switch (c) {
		case '"': case '[': case '{':
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9':
			us_ungetc(ds->ds_us, c);
			ch = parse_datafield(ds);
			if (ch == NULL) {
				datafield_free(df);
				return (NULL);
			}
			psc_dynarray_push(&df->df_array, ch);
			break;
		case ',':
		case ']':
			break;
		default:
			PARSE_ERROR(ds, "");
			datafield_free(df);
			return (NULL);
		}
	} while (c != ']');
	skip_data_space(ds);
	return (df);
}

struct datafield *
parse_datafield_map(struct datasrc *ds)
{
	struct datafield *df, *ch;
	int c;

	c = us_getc(ds->ds_us);
	if (c != '{') {
		PARSE_ERROR(ds, "invalid char '%c'", c);
		return (NULL);
	}
	df = datafield_new(DFT_MAP);
	do {
		skip_data_space(ds);
		c = us_getc(ds->ds_us);
		switch (c) {
		case '"':
			ch = parse_datafield_mapentry(ds);
			if (ch == NULL) {
				PARSE_ERROR(ds, "");
				datafield_free(df);
				return (NULL);
			}

			psc_dynarray_push(&df->df_mapfields, ch);
			c = us_getc(ds->ds_us);
			switch (c) {
			case '}':
			case ',':
				break;
			default:
				PARSE_ERROR(ds, "");
				datafield_free(df);
				return (NULL);
			}
			break;
		case '}':
			break;
		default:
			PARSE_ERROR(ds, "");
			datafield_free(df);
			return (NULL);
		}
	} while (c != '}');
	skip_data_space(ds);
	return (df);
}

struct datafield *
parse_datafield(struct datasrc *ds)
{
	struct datafield *df;
	int c;

	skip_data_space(ds);
	c = us_getc(ds->ds_us);
	switch (c) {
	case '0': case '1': case '2':
	case '3': case '4': case '5':
	case '6': case '7': case '8':
	case '9': case '"':
		us_ungetc(ds->ds_us, c);
		df = parse_datafield_scalar(ds);
		break;
	case '[':
		us_ungetc(ds->ds_us, c);
		df = parse_datafield_array(ds);
		break;
	case '{':
		us_ungetc(ds->ds_us, c);
		df = parse_datafield_map(ds);
		break;
	default:
		PARSE_ERROR(ds, "invalid char '%c'", c);
		return (NULL);
	}
	skip_data_space(ds);
	return (df);
}

struct datafield *
parse_datafield_mapentry(struct datasrc *ds)
{
	struct datafield *df = NULL;
	struct buf namebuf;
	int c;

	buf_init(&namebuf);
	do {
		c = us_getc(ds->ds_us);
		switch (c) {
		case '"':
			break;
		case '\0':
			PARSE_ERROR(ds, "");
			buf_free(&namebuf);
			return (NULL);
		default:
			buf_append(&namebuf, c);
			break;
		}
	} while (c != '"');

	buf_nul(&namebuf);
	skip_data_space(ds);
	c = us_getc(ds->ds_us);
	if (c != ':') {
		PARSE_ERROR(ds, "");
		buf_free(&namebuf);
		return (NULL);
	}
	df = parse_datafield(ds);
	if (df == NULL) {
		buf_free(&namebuf);
		return (NULL);
	}
	df->df_name = buf_get(&namebuf);
	skip_data_space(ds);
	return (df);
}

int
datafield_map_getscalarint(struct datafield *df, const char *name,
    int *ip)
{
	const char *s;
	char *endp;
	long l;

	s = datafield_map_getscalar(df, name);
	if (s == NULL)
		return (-1);
	l = strtol(s, &endp, 10);
	if (l <= INT_MIN || l >= INT_MAX || endp == s || *endp)
		return (-1);
	*ip = l;
	return (0);
}

int
apply_node(struct datafield *node)
{
	struct physcoord pc;
	struct node *n;
	const char *s;
	int nid, cfg;

	memset(&pc, 0, sizeof(pc));
	if (datafield_map_getscalarint(node, "partition_num", &pc.pc_part))
		return (-1);
	if (datafield_map_getscalarint(node, "rack", &pc.pc_rack))
		return (-1);
	if (datafield_map_getscalarint(node, "iru", &pc.pc_iru))
		return (-1);
	if (datafield_map_getscalarint(node, "blade", &pc.pc_blade))
		return (-1);
	if (datafield_map_getscalarint(node, "node", &pc.pc_node))
		return (-1);
	if (datafield_map_getscalarint(node, "memnid", &nid))
		return (-1);
	if (datafield_map_getscalarint(node, "configured", &cfg))
		return (-1);

	pc.pc_part--;
	pc.pc_rack--;
	pc.pc_iru = pc.pc_iru == 1 ? 0 : 1;

	s = datafield_map_getscalar(node, "comment");

	n = node_for_pc(&pc);
	if (n == NULL || nid > machine.m_nidmax)
		return (-1);
	n->n_nid = nid;
	node_nidmap[nid] = n;
	if (cfg)
		n->n_state = SC_FREE;
	else if (s && strcmp(s, "boot") == 0)
		n->n_state = SC_BOOT;
	else if (s && strcmp(s, "IB card") == 0)
		n->n_state = SC_IO;
	n->n_flags |= NF_VALID;
	return (0);
}

void
nidlist_getnextrange(const char **s, int *min, int *max)
{
	const char *p;
	char *endp;
	long l;

	*min = 0;
	*max = -1;

	l = strtol(*s, &endp, 10);
	if (l <= 0 || l >= INT_MAX || endp == *s || *endp != '-')
		return;
	*min = l;

	l = strtol(endp + 1, &endp, 10);
	if (l <= 0 || l >= INT_MAX || endp == *s ||
	    (*endp && *endp != ','))
		return;
	*max = l;

	p = endp;
	*s = p + 1;
}

/* 1168-1295,3216-3343 */
#define FOREACH_NIDINLIST(nid, min, max, s, nidlist)			\
	for ((s) = (nidlist); (s); )					\
		for (nidlist_getnextrange(&(s), &(min), &(max)),	\
		    (nid) = (min); (nid) <= (max); (nid)++)

/*
 * 96:00:00
 */
int
parse_time(const char *s)
{
	char *endp;
	int n, l;

	l = strtol(s, &endp, 10);
	if (l < 0 || l >= INT_MAX || endp == s || *endp != ':')
		return (0);
	n = l * 60 * 60;

	l = strtol(endp + 1, &endp, 10);
	if (l < 0 || l >= INT_MAX || endp == s || *endp != ':')
		return (0);
	n += l * 60;

	l = strtol(endp + 1, &endp, 10);
	if (l < 0 || l >= INT_MAX || endp == s || *endp != '\0')
		return (0);
	n += l;
	return (n);
}

__inline int
apply_job(struct datafield *job)
{
	int jobid, nid, min, max;
	const char *s, *nidlist;
	struct datafield *df;
	struct node *n;
	struct job *j;
	char *endp;
	long l;

	/*
		"exec_host":"bl0.psc.teragrid.org",
		"Resource_List":{
			"walltime_min":"00:00:00",
			"walltime":"96:00:00",
			"walltime_max":"00:00:00",
			"pnum_threads_factor":"16",
			"ncpus":"128",
			"nodeset":"146-161:1168-1295,3216-3343"
		},
		"Job_Name":"141-3-3-4",
		"Job_Owner":"mhutchin@tg-login1.blacklight.psc.teragrid.org",
		"qtime":"1302534343",
		"resources_used":{
			"walltime":"26:27:42",
			"cput":"2189:29:24",
			"mem":"105132944kb",
			"vmem":"269585638652kb"
		},
		"ctime":"1302330285",
		"mtime":"1302534345",
		"start_time":"1302534343",
		"etime":"1302330285",
		"comment":"Starting job at 04/11/11 11:05",
		"queue":"batch_l",
		"Job_Id":"32254.tg-login1.blacklight.psc.teragrid.org",
	 */

	s = datafield_map_getscalar(job, "Job_Id");
	if (s == NULL) {
		warnx("job ID not present");
		return (0);
	}
	l = strtol(s, NULL, 10);
	if (l <= 0 || l >= INT_MAX) {
		warnx("job ID not present");
		return (0);
	}
	jobid = l;

	j = n->n_job = obj_get(&jobid, &job_list);
	j->j_id = jobid;
	if (strcmp(j->j_name, "") == 0)
		snprintf(j->j_name, sizeof(j->j_name), "job %d", jobid);

	s = datafield_map_getscalar(job, "owner");
	if (s) {
		strncpy(j->j_owner, s, sizeof(j->j_owner) - 1);
		j->j_owner[sizeof(j->j_owner) - 1] = '\0';
		endp = strchr(j->j_owner, '@');
		if (endp)
			*endp = '\0';
	}
	s = datafield_map_getscalar(job, "queue");
	if (s) {
		strncpy(j->j_queue, s, sizeof(j->j_queue));
		j->j_queue[sizeof(j->j_queue) - 1] = '\0';
	}
	s = datafield_map_getscalar(job, "Job_Name");
	if (s) {
		strncpy(j->j_name, s, sizeof(j->j_name));
		j->j_name[sizeof(j->j_name) - 1] = '\0';
	}

	/* apply data from Resource_List */
	df = datafield_map_getkeyvalue(job, "Resource_List");
	if (df == NULL || df->df_type != DFT_MAP)
		return (0);
	s = datafield_map_getscalar(df, "ncpus");
	if (s) {
		l = strtol(s, &endp, 10);
		if (l >= 0 && l < INT_MAX && endp != s && *endp)
			j->j_ncpus = l;
	}
	s = datafield_map_getscalar(df, "walltime");
	if (s)
		j->j_tmdur = parse_time(s);

	s = datafield_map_getscalar(job, "nodeset");
	if (s)
		FOREACH_NIDINLIST(nid, min, max, s, nidlist) {
			n = node_for_nid(nid);
			if (n == NULL) {
				warnx("nid %d not found", nid);
				continue;
			}
			if (n->n_state == SC_FREE)
				n->n_state = SC_ALLOC;
			else
				warnx("node state not FREE");
		}

	/* apply data from resources_used */
	df = datafield_map_getkeyvalue(job, "resources_used");
	if (df == NULL || df->df_type != DFT_MAP)
		return (0);
	s = datafield_map_getscalar(df, "mem");
	if (s) {
		l = strtol(s, &endp, 10);
		if (l >= 0 && l < INT_MAX && endp != s && *endp == 'k')
			j->j_mem = l;
	}
	s = datafield_map_getscalar(df, "walltime");
	if (s)
		j->j_tmuse = parse_time(s);
	return (0);
}

#define DATA_FORM_ERROR(fmt, ...)					\
	do {								\
		warnx("data form unexpected format (source line %d): "	\
		    fmt, __LINE__, ##__VA_ARGS__);			\
		goto out;						\
	} while (0)

void
datafield_print(struct datafield *df, int lvl)
{
	struct datafield *ch;
	int i;

	switch (df->df_type) {
	case DFT_SCALAR:
		printf("\"%s\"", buf_get(&df->df_scalar));
		break;
	case DFT_ARRAY:
		printf("[");
		DYNARRAY_FOREACH(ch, i, &df->df_array) {
			if (i)
				printf(",");
			if (lvl < 4)
				printf("\n%*s", lvl, "");
			datafield_print(ch, lvl + 1);
		}
		printf("]");
		break;
	case DFT_MAP:
		printf("{");
		DYNARRAY_FOREACH(ch, i, &df->df_mapfields) {
			if (i)
				printf(",");
			if (lvl < 4)
				printf("\n%*s", lvl, "");
			printf("\"%s\":", ch->df_name);
			datafield_print(ch, lvl + 1);
		}
		printf("}");
		break;
	}
}

/*
 * data = {"version":"1.1","result":{
 *	"jobs":[{"session_id":"174067",...},...],
 *	"history":[{"session_id":"174004",...},...],
 *	"queue":[{"Job_Name":"12s-4-4-1",...},...],
 *	"sysinfo":{"mempercpu":8,"gb_per_memnode":64,"mem":16384}}}
*/
void
parse_data(struct datasrc *ds)
{
	struct datafield *data = NULL, *r, *sysinfo, *node, *nodev, *jobs, *job;
	struct node *n, **np;
	struct job *j;
	size_t k;
	int i;

	if (skip_data_strlit(ds, "data"))
		goto error;
	skip_data_space(ds);
	if (skip_data_strlit(ds, "="))
		goto error;
	skip_data_space(ds);
	data = parse_datafield_map(ds);
	if (data == NULL)
		goto error;

	if (us_sawerror(ds->ds_us))
		warnx("read: %s", us_errstr(ds->ds_us));

	NODE_FOREACH_PHYS(n, np) {
		n->n_flags &= ~NF_VALID;
		n->n_state = SC_NOTSCHED;
		n->n_job = NULL;
	}

	r = datafield_map_getkeyvalue(data, "result");
	if (r == NULL || r->df_type != DFT_MAP)
		DATA_FORM_ERROR("");

	sysinfo = datafield_map_getkeyvalue(r, "sysinfo");
	if (sysinfo == NULL || sysinfo->df_type != DFT_MAP)
		DATA_FORM_ERROR("sysinfo wrong type=%d", sysinfo->df_type);

	jobs = datafield_map_getkeyvalue(r, "jobs");
	if (jobs == NULL || jobs->df_type != DFT_ARRAY)
		DATA_FORM_ERROR();

	nodev = datafield_map_getkeyvalue(sysinfo, "nodes");
	if (nodev == NULL || nodev->df_type != DFT_ARRAY)
		DATA_FORM_ERROR();

	DYNARRAY_FOREACH(node, i, &nodev->df_array)
		if (node->df_type != DFT_MAP || apply_node(node))
			DATA_FORM_ERROR("node is wrong type=%d", node->df_type);

	DYNARRAY_FOREACH(job, i, &jobs->df_array)
		if (job->df_type != DFT_MAP || apply_job(job))
			DATA_FORM_ERROR();

 out:
	for (k = 0; k < job_list.ol_tcur; k++) {
		j = OLE(job_list, k, job);
		col_get_hash(&j->j_oh, j->j_id, &j->j_fill);
	}

	if (data)
		datafield_free(data);

	errno = 0;

	if (0) {
 error:
		warnx("parse error");
	}
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
