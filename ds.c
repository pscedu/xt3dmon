/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "mon.h"

#define _RPATH_TEMP	"/xt3-data/temps"
#define _RPATH_PHYS	"/xt3-data/rtrtrace"
#define _RPATH_JOBS	"/xt3-data/nids_list_phantom"
#define _RPATH_BAD	"/xt3-data/bad_nids_list_phantom"
#define _RPATH_CHECK	"/xt3-data/to_check_nids_list_phantom"
#define _RPATH_QSTAT	"/xt3-data/qstat.out"
#define _RPATH_FAIL	"/xt3-data/fails"

#define RDS_HOST	"mugatu"
#define RDS_PORT	80

int ds_http(const char *);

#ifndef _LIVE_DSP
# define _LIVE_DSP DSP_REMOTE
#endif

struct datasrc datasrcs[] = {
	{ "temp", 0, DSF_AUTO, DSP_LOCAL, _PATH_TEMPMAP,  _RPATH_TEMP,  parse_tempmap,	db_tempmap,	&temp_list,	{ 0 } },
	{ "phys", 0, DSF_AUTO, DSP_LOCAL, _PATH_PHYSMAP,  _RPATH_PHYS,  parse_physmap,	db_physmap,	NULL,		{ 0 } },
	{ "jobs", 0, DSF_AUTO, _LIVE_DSP, _PATH_JOBMAP,   _RPATH_JOBS,  parse_jobmap,	db_jobmap,	&job_list,	{ 0 } },
	{ "bad",  0, DSF_AUTO, DSP_LOCAL, _PATH_BADMAP,   _RPATH_BAD,   parse_badmap,	db_badmap,	NULL,		{ 0 } },
	{ "check",0, DSF_AUTO, DSP_LOCAL, _PATH_CHECKMAP, _RPATH_CHECK, parse_checkmap,	db_checkmap,	NULL,		{ 0 } },
	{ "qstat",0, DSF_AUTO, _LIVE_DSP, _PATH_QSTAT,    _RPATH_QSTAT, parse_qstat,	db_qstat,	NULL,		{ 0 } },
	{ "mem",  0, DSF_AUTO, DSP_LOCAL, _PATH_STAT,     NULL,		parse_mem,	NULL,		NULL,		{ 0 } },
	{ "fail", 0, DSF_AUTO, DSP_LOCAL, _PATH_FAILMAP,  _RPATH_FAIL,  parse_failmap,	db_failmap,	&fail_list,	{ 0 } }
};
#define NDATASRCS (sizeof(datasrcs) / sizeof(datasrcs[0]))

void
ds_close(struct datasrc *ds)
{
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
	case DSP_REMOTE:
		if (ds->ds_fd != -1)
			close(ds->ds_fd);
		break;
	}
}

void
ds_read(struct datasrc *ds)
{
	if (ds->ds_objlist)
		obj_batch_start(ds->ds_objlist);
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
	case DSP_REMOTE:
		ds->ds_parsef(ds);
		break;
	case DSP_DB:
		ds->ds_dbf(ds);
		break;
	}
	if (ds->ds_objlist)
		obj_batch_end(ds->ds_objlist);
}

void
ds_chdsp(int type, int dsp)
{
	struct datasrc *ds;

	ds = &datasrcs[type];
	if (ds->ds_dsp == dsp)
		return;
	ds->ds_dsp = dsp;
	ds->ds_mtime = 0;
	ds->ds_flags = 0;
}

__inline struct datasrc *
ds_get(int type)
{
	return (&datasrcs[type]);
}

void
ds_refresh(int type, int flags)
{
	struct datasrc *ds;
	struct stat st;
	int readit;

	readit = 1;
	ds = ds_open(type);
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
		if (ds->ds_fd == -1) {
			readit = 0;
			break;

		}
		if (fstat(ds->ds_fd, &st) == -1)
			err(1, "fstat %s", ds->ds_lpath);
		/* XXX: no way to tell if it was modified with <1 second resolution. */
		if (st.st_mtime <= ds->ds_mtime &&
		    (ds->ds_flags & DSF_FORCE) == 0) {
			readit = 0;
			break;
		}
		ds->ds_mtime = st.st_mtime;
		if ((ds->ds_flags & DSF_AUTO) == 0) {
//			ds->ds_flags |= DSF_READY;
			status_add("New data available");
			readit = 0;
			break;
		}
		break;
	}
	if (readit) {
		ds->ds_flags &= ~DSF_FORCE;
		ds_read(ds);
	} else {
		switch (ds->ds_dsp) {
		case DSP_LOCAL:
		case DSP_REMOTE:
			if (ds->ds_fd == -1) {
				if (flags & DSF_CRIT)
					err(1, "datasrc (%s) open failed",
					    ds->ds_name);
				else if ((flags & DSF_IGN) == 0)
					warn("datasrc (%s) open failed",
					    ds->ds_name);
			}
			break;
		}
	}
	ds_close(ds);
}

struct datasrc *
ds_open(int type)
{
	struct datasrc *ds;
	int mod;

	mod = 0;
	ds = ds_get(type);
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
		ds->ds_fd = open(ds->ds_lpath, O_RDONLY);
		break;
	case DSP_REMOTE:
		ds->ds_fd = ds_http(ds->ds_rpath);
		break;
	}
	return (ds);
}

int
ds_http(const char *path)
{
	struct http_req r;

	if (path == NULL)
		errx(1, "no remote data available for datasrc type");
	memset(&r, 0, sizeof(r));
	r.htreq_server = RDS_HOST;
	r.htreq_port = RDS_PORT;

	r.htreq_method = "GET";
	r.htreq_version = "HTTP/1.1";
	r.htreq_url = path;
	return (http_open(&r, NULL));
}

__inline void
dsc_fn(struct datasrc *ds, const char *sid, char *buf, size_t len)
{
	snprintf(buf, len, "%s/%s/%s", _PATH_SESSIONS, sid,
	    ds->ds_name);
}

void
dsc_create(const char *sid)
{
	char fn[PATH_MAX];

	snprintf(fn, sizeof(fn), "%s/%s", _PATH_SESSIONS, sid);
	if (mkdir(fn, 0755) == -1)
		err(1, "mkdir: %s", fn);
}

int
dsc_exists(const char *sid)
{
	char fn[PATH_MAX];
	struct stat st;

	snprintf(fn, sizeof(fn), "%s/%s", _PATH_SESSIONS, sid);
	if (stat(fn, &st) == -1) {
		/* XXX: more robust, check structure & dir */
		if (errno == ENOENT)
			return (0);
		else
			err(1, "stat");
	}
	return (1);
}

void
dsc_clone(int type, const char *sid)
{
	char buf[BUFSIZ], fn[BUFSIZ];
	struct datasrc *ds;
	ssize_t n;
	int fd;

	if (!dsc_exists(sid))
		dsc_create(sid);

	ds = ds_open(type);
	dsc_fn(ds, sid, fn, sizeof(fn));
	if ((fd = open(fn, O_RDWR | O_CREAT, 0644)) == -1)
		err(1, "open: %s", fn);

	switch (ds->ds_dsp) {
	case DSP_LOCAL:
	case DSP_REMOTE:
		while ((n = read(ds->ds_fd, &buf, sizeof(buf))) != -1 &&
		    n != 0)
			if (write(fd, buf, n) != n)
				err(1, "write");
		if (n == -1)
			err(1, "read");
		break;
	case DSP_DB:
		/* XXX: not implemented */
		break;
	}
	close(fd);
	ds_close(ds);
}

struct datasrc *
dsc_open(int type, const char *sid)
{
	struct datasrc *ds;
	char fn[PATH_MAX];

	ds = ds_get(type);
	snprintf(fn, sizeof(fn), "%s/%s/%s", _PATH_SESSIONS, sid,
	    ds->ds_name);
	if ((ds->ds_fd = open(fn, O_RDONLY, 0)) == -1)
		return (NULL);
	return (ds);
}

__inline void
dsc_close(struct datasrc *ds)
{
	close(ds->ds_fd);
}

void
dsc_load(int type, const char *sid)
{
	struct datasrc *ds;

	ds = dsc_open(type, sid);
	if (ds == NULL)
		err(1, "dsc_open");
	ds_read(ds);
	dsc_close(ds);
}

int
st_dsmode(void)
{
	int ds = -1;

	switch (st.st_mode) {
	case SM_JOBS:
		ds = DS_JOBS;
		break;
	case SM_FAIL:
		ds = DS_FAIL;
		break;
	case SM_TEMP:
		ds = DS_TEMP;
		break;
	}
	return (ds);
}
