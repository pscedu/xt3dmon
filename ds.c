/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "cdefs.h"
#include "mon.h"
#include "ustream.h"

#define _RPATH_NODE	"/xt3-data/node"
#define _RPATH_JOB	"/xt3-data/job"
#define _RPATH_YOD	"/xt3-data/yod"

#define RDS_HOST	"mugatu.psc.edu"
#define RDS_PORT	80

struct ustream *ds_http(const char *);

#ifndef _LIVE_DSP
# define _LIVE_DSP DSP_REMOTE
#endif

/* Must match DS_* defines in mon.h. */
struct datasrc datasrcs[] = {
	{ "node", 0, DSF_AUTO, _LIVE_DSP, _PATH_NODE,  _RPATH_NODE, parse_node,	dsfi_node, dsff_node,	0 },
	{ "job",  0, DSF_AUTO, _LIVE_DSP, _PATH_JOB,   _RPATH_JOB,  parse_job,	NULL,	   NULL,	0 },
	{ "yod",  0, DSF_AUTO, _LIVE_DSP, _PATH_YOD,   _RPATH_YOD,  parse_yod,	NULL,	   NULL,	0 },
	{ "mem",  0, DSF_AUTO, DSP_LOCAL, _PATH_STAT,  NULL,	    parse_mem,	NULL,	   NULL,	0 },
};
#define NDATASRCS (sizeof(datasrcs) / sizeof(datasrcs[0]))

void
dsfi_node(__unused struct datasrc *ds)
{
	obj_batch_start(&job_list);
	obj_batch_start(&yod_list);
}

void
dsff_node(__unused struct datasrc *ds)
{
	obj_batch_end(&job_list);
	obj_batch_end(&yod_list);
}

void
ds_close(struct datasrc *ds)
{
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
	case DSP_REMOTE:
		us_close(ds->ds_us);
		break;
	}
}

void
ds_read(struct datasrc *ds)
{
	if (ds->ds_initf)
		ds->ds_initf(ds);
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
	case DSP_REMOTE:
		ds->ds_parsef(ds);
		break;
	}
	if (ds->ds_finif)
		ds->ds_finif(ds);
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

	ds = ds_open(type);

	if (ds == NULL) {
		if (flags & DSF_CRIT)
			err(1, "datasrc (%s) open failed", ds->ds_name);
		else if ((flags & DSF_IGN) == 0)
			warn("datasrc (%s) open failed", ds->ds_name);
		return;
	}

	readit = 1;
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
		if (fstat(ds->ds_us->us_fd, &st) == -1)
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
	}

	ds_close(ds);
}

struct datasrc *
ds_open(int type)
{
	struct datasrc *ds;
	int mod, fd;

	mod = 0;
	ds = ds_get(type);
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
		if ((fd = open(ds->ds_lpath, O_RDONLY)) == -1)
			return (NULL);
		ds->ds_us = us_init(fd, UST_FILE, "r");
		break;
	case DSP_REMOTE:
		if ((ds->ds_us = ds_http(ds->ds_rpath)) == NULL)
			return (NULL);
		break;
	}
	return (ds);
}

struct ustream *
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
		/* XXXXXXXX: use us_read */
		while ((n = read(ds->ds_us->us_fd, &buf,
		    sizeof(buf))) != -1 && n != 0)
			if (write(fd, buf, n) != n)
				err(1, "write");
		if (n == -1)
			err(1, "read");
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
	int fd;

	ds = ds_get(type);
	snprintf(fn, sizeof(fn), "%s/%s/%s", _PATH_SESSIONS, sid,
	    ds->ds_name);
	if ((fd = open(fn, O_RDONLY, 0)) == -1)
		return (NULL);
	ds->ds_us = us_init(fd, UST_FILE, "r");
	return (ds);
}

__inline void
dsc_close(struct datasrc *ds)
{
	us_close(ds->ds_us);
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
	int ds = DS_INV;

	switch (st.st_mode) {
	case SM_JOB:
		ds = DS_JOB;
		break;
	case SM_YOD:
		ds = DS_YOD;
		break;
	}
	return (ds);
}
