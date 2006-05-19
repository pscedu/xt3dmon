/* $Id$ */

#include "mon.h"

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cdefs.h"
#include "ds.h"
#include "http.h"
#include "objlist.h"
#include "panel.h"
#include "pathnames.h"
#include "state.h"
#include "ustream.h"
#include "util.h"

#define _RPATH_NODE	"/arbiter-raw.pl?data=nodes"
#define _RPATH_JOB	"/arbiter-raw.pl?data=jobs"
#define _RPATH_YOD	"/arbiter-raw.pl?data=yods"
#define _RPATH_RT	"/arbiter-raw.pl?data=rt"
#define _RPATH_SS	"/arbiter-raw.pl?data=ss"

#define RDS_HOST	"bigben-monitor.psc.edu"
#define RDS_PORT	80
#define RDS_PORTSSL	443

#ifndef _LIVE_DSP
# define _LIVE_DSP DSP_REMOTE
#endif

/* Must match DS_* defines in ds.h. */
struct datasrc datasrcs[] = {
	{ "node", 0, _LIVE_DSP, _PATH_NODE, _RPATH_NODE, parse_node, DSF_AUTO | DSF_USESSL, dsfi_node,	dsff_node,  NULL },
	{ "job",  0, _LIVE_DSP, _PATH_JOB,  _RPATH_JOB,  parse_job,  DSF_AUTO | DSF_USESSL, NULL,	NULL,	    NULL },
	{ "yod",  0, _LIVE_DSP, _PATH_YOD,  _RPATH_YOD,  parse_yod,  DSF_AUTO | DSF_USESSL, NULL,	NULL,	    NULL },
	{ "rt",   0, _LIVE_DSP, _PATH_RT,   _RPATH_RT,   parse_rt,   DSF_AUTO | DSF_USESSL, NULL,	NULL,       NULL },
	{ "mem",  0, DSP_LOCAL, _PATH_STAT, NULL,	 parse_mem,  DSF_AUTO,		    NULL,	NULL,	    NULL },
	{ "ss",   0, _LIVE_DSP, _PATH_SS,   _RPATH_SS,   parse_ss,   DSF_AUTO | DSF_USESSL, NULL,	NULL,       NULL }
};

int	 dsp = DSP_LOCAL;
int	 dsflags = DSFF_ALERT;

void
dsfi_node(__unused const struct datasrc *ds)
{
	obj_batch_start(&job_list);
	obj_batch_start(&yod_list);
}

void
dsff_node(__unused const struct datasrc *ds)
{
	obj_batch_end(&job_list);
	obj_batch_end(&yod_list);
}

__inline void
ds_close(struct datasrc *ds)
{
	us_close(ds->ds_us);
}

void
ds_read(struct datasrc *ds)
{
	if (ds->ds_initf)
		ds->ds_initf(ds);
	ds->ds_parsef(ds);
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

void
ds_refresh(int type, int flags)
{
	struct datasrc *ds;
	struct stat st;
	int readit;

	ds = ds_open(type);
	if (ds == NULL) {
		if (flags & DSFF_CRIT)
			err(1, "datasrc (%d) open failed", type);
		else if (flags & DSFF_ALERT) {
			status_add("Unable to retrieve data: %s\n", strerror(errno));
			panel_show(PANEL_STATUS);
		} else if ((flags & DSFF_IGN) == 0)
			warn("datasrc (%d) open failed", type);
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

struct ustream *
ds_http(const char *path, struct http_res *res)
{
	struct http_req req;

	if (path == NULL)
		errx(1, "no remote data available for datasrc type");
	memset(&req, 0, sizeof(req));
	req.htreq_server = RDS_HOST;
	req.htreq_port = RDS_PORT;

	req.htreq_method = "GET";
	req.htreq_version = "HTTP/1.1";
	req.htreq_url = path;
	return (http_open(&req, res));
}

struct ustream *
ds_https(const char *path, struct http_res *res)
{
	struct ustream *usp;
	struct http_req req;

	if (path == NULL)
		errx(1, "no remote data available for datasrc type");
	memset(&req, 0, sizeof(req));
	req.htreq_server = RDS_HOST;
	req.htreq_port = RDS_PORTSSL;

	req.htreq_method = "GET";
	req.htreq_version = "HTTP/1.1";
	req.htreq_url = path;

	if ((req.htreq_extra = calloc(2, sizeof(char *))) == NULL)
		err(1, "calloc");
	if (asprintf(&req.htreq_extra[0], "Authorization: Basic %s\r\n",
	    login_auth) == -1)
		err(1, "asprintf");

	usp = http_open(&req, res);

	free(req.htreq_extra[0]);
	free(req.htreq_extra);
	return (usp);
}

struct datasrc *
ds_open(int type)
{
	struct ustream *(*httpf)(const char *, struct http_res *);
	struct http_res res;
	struct datasrc *ds;
	int mod, fd;

	mod = 0;
	ds = &datasrcs[type];
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
		if ((fd = open(ds->ds_lpath, O_RDONLY)) == -1)
			return (NULL);
		ds->ds_us = us_init(fd, UST_LOCAL, "r");
		break;
	case DSP_REMOTE:
		httpf = ds_http;
		if (login_auth[0] != '\0')
			httpf = ds_https;

		memset(&res, 0, sizeof(res));
		if ((ds->ds_us = httpf(ds->ds_rpath, &res)) == NULL)
			return (NULL);
		if (res.htres_code != 200) {
			us_close(ds->ds_us);
			switch (res.htres_code) {
			case 401:
				errno = EACCES;
				break;
			case 404:
				errno = ENOENT;
				break;
			default:
				errno = EAGAIN;
				break;
			}
			return (NULL);
		}
		break;
	}
	return (ds);
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
	if (mkdir(fn, 0755) == -1) {
		/* Check that sessions directory exists. */
		if (errno == ENOENT) {
			if (mkdir(_PATH_SESSIONS, 01755) == -1)
				err(1, "mkdir: %s", _PATH_SESSIONS);
			if (mkdir(fn, 0755) != -1) {
				errno = 0;
				return;
			}
		}
		err(1, "mkdir: %s", fn);
	}
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

	ds = &datasrcs[type];
	snprintf(fn, sizeof(fn), "%s/%s/%s", _PATH_SESSIONS, sid,
	    ds->ds_name);
	if ((fd = open(fn, O_RDONLY, 0)) == -1)
		return (NULL);
	ds->ds_us = us_init(fd, UST_LOCAL, "r");
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

	switch (st.st_dmode) {
	case DM_JOB:
		ds = DS_JOB;
		break;
	case DM_YOD:
		ds = DS_YOD;
		break;
	}
	return (ds);
}
