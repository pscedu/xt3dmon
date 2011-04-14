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

/*
 * Data source routines.
 *
 * Performs all file handling and such for retrieving data,
 * from various data sources (such as client sessions when
 * running in server mode).
 *
 * For server mode, routines are also provided for cloning
 * data sets (dsc_*).
 */

#include "mon.h"

#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cdefs.h"
#include "buf.h"
#include "ds.h"
#include "http.h"
#include "objlist.h"
#include "panel.h"
#include "parse.h"
#include "pathnames.h"
#include "state.h"
#include "status.h"
#include "ustream.h"
#include "util.h"

#define MAX_PROTO_LEN	20
#define MAX_PORT_LEN	6
#define MAX_PATH_LEN	BUFSIZ

/* Must match DS_* defines in ds.h. */
struct datasrc datasrcs[] = {
	{ "data", 0, "", parse_data, DSF_LIVE,	dsfi_node, dsff_node,	NULL },
};

struct objlist ds_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };

int	 dsfopts = DSFO_ALERT | DSFO_LIVE;
char	 ds_browsedir[PATH_MAX] = _PATH_ARCHIVE;
char	 ds_dir[PATH_MAX] = _PATH_DATADIR;
char	 ds_datasrc[PATH_MAX] = "http://mugatu.psc.edu:24240/UView";

__inline int
uri_has_proto(const char *uri, const char *proto)
{
	size_t len;

	len = strlen(proto);
	return (strncmp(uri, proto, len) == 0 && uri[len] == ':');
}

void
ds_setlive(void)
{
	struct buf cache, live;
	int j;

	buf_init(&cache);
	buf_appendv(&cache, "file://");
	escape_printf(&cache, _PATH_DATADIR);
	buf_appendv(&cache, "/%s");
	buf_nul(&cache);

	buf_init(&live);
	if (strncmp(ds_datasrc, "http:", 5) == 0 &&
	    (dsfopts & DSFO_SSL)) {
		buf_appendv(&live, "https");
		buf_appendv(&live, ds_datasrc + 4);
	} else
		buf_appendv(&live, ds_datasrc);
	buf_nul(&live);

	for (j = 0; j < NDS; j++)
		snprintf(datasrcs[j].ds_uri, sizeof(datasrcs[j].ds_uri),
		    buf_get(datasrcs[j].ds_flags & DSF_LIVE ? &live : &cache),
		    datasrcs[j].ds_name);

	buf_free(&cache);
	buf_free(&live);

	st.st_rf |= RF_DATASRC;
	dsfopts |= DSFO_LIVE;
	panel_rebuild(PANEL_DSCHO);
}

/*
 * Since parse_node uses obj_get() for creating/finding
 * jobs, we protect the job lists when
 * starting and finalizing node data source fetches.
 */
void
dsfi_node(__unusedx const struct datasrc *ds)
{
	obj_batch_start(&job_list);
}

void
dsff_node(__unusedx const struct datasrc *ds)
{
	obj_batch_end(&job_list);
}

__inline void
ds_close(struct datasrc *ds)
{
	us_close(ds->ds_us);
}

int
uri_parse(const char *s, char *proto, char *host, char *port, char *path)
{
	int i;

	for (i = 0; i < MAX_PROTO_LEN - 1 && isalpha(*s); s++, i++)
		proto[i] = *s;
	proto[i] = '\0';

	if (*s++ != ':')
		return (0);
	if (*s++ != '/')
		return (0);
	if (*s++ != '/')
		return (0);

	if (strcmp(proto, "file") == 0) {
		host[0] = '\0';
		port[0] = '\0';
	} else {
		for (i = 0; i < MAXHOSTNAMELEN - 1 &&
		    (isalnum(*s) || *s == '.' || *s == '-'); s++, i++)
			host[i] = *s;
		host[i] = '\0';

		i = 0;
		if (*s == ':') {
			s++;
			for (; i < MAX_PORT_LEN - 1 &&
			    isdigit(*s); s++, i++)
				port[i] = *s;
		}
		port[i] = '\0';
	}

	for (i = 0; i < MAX_PATH_LEN - 1; s++, i++)
		path[i] = *s;
	path[i] = '\0';

	return (1);
}

void
ds_seturi(int dsid, const char *urifmt)
{
	struct datasrc *ds;

	ds = &datasrcs[dsid];
	snprintf(ds->ds_uri, sizeof(ds->ds_uri), urifmt, ds->ds_name);
	ds->ds_flags |= DSF_FORCE;
	st.st_rf |= RF_DATASRC;
	dsfopts &= ~DSFO_LIVE;
}

char *
ds_set(const char *fn, int flags)
{
	char path[PATH_MAX];
	struct stat stb;
	struct buf buf;
	int j;

	if ((flags & CHF_DIR) == 0)
		errx(1, "file chosen when directory-only");

	panel_rebuild(PANEL_DSCHO);
	snprintf(path, sizeof(path), "%s/%s/%s",
	    ds_browsedir, fn, _PATH_ISDUMP);
	if (stat(path, &stb) == -1) {
		if (errno != ENOENT)
			err(1, "stat %s", path);
		errno = 0;
	} else {
		/* Dataset exists, don't descend. */
		if (strcmp(fn, "..") != 0) {
			snprintf(ds_dir, sizeof(ds_dir),
			    "%s/%s", ds_browsedir, fn);
			buf_init(&buf);
			buf_appendv(&buf, "file://");
			escape_printf(&buf, ds_dir);
			buf_appendv(&buf, "/%s");
			buf_nul(&buf);
			for (j = 0; j < NDS; j++)
				ds_seturi(j, buf_get(&buf));
			buf_free(&buf);
			return (NULL);
		}
	}
	return (ds_browsedir);
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
ds_refresh(int type, int flags)
{
	struct datasrc *ds;
	struct stat stb;

	ds = ds_open(type);
	if (ds == NULL) {
		if (flags & DSFO_CRIT)
			err(1, "datasrc (%d) open failed", type);
		else if (flags & DSFO_ALERT) {
			status_add(SLP_URGENT,
			    "Unable to retrieve data: %s\n",
			    strerror(errno));
			panel_show(PANEL_STATUS);
		} else if ((flags & DSFO_IGN) == 0)
			warn("datasrc (%d) open failed", type);
		return;
	}

	if (uri_has_proto(ds->ds_uri, "file")) {
		if (fstat(ds->ds_us->us_fd, &stb) == -1)
			err(1, "fstat %s", ds->ds_uri);
		/*
		 * XXX: no way to tell if it was modified
		 *	with <1 second resolution.
		 */
		if (stb.st_mtime <= ds->ds_mtime &&
		    (ds->ds_flags & DSF_FORCE) == 0)
			goto done;
		ds->ds_mtime = stb.st_mtime;
	}
	ds->ds_flags &= ~DSF_FORCE;
	ds_read(ds);
 done:
	ds_close(ds);
}

struct ustream *
ds_http(const char *server, const char *port, const char *path,
    struct http_res *res, __unusedx int flags)
{
	struct http_req req;

	if (path == NULL)
		errx(1, "no remote data available for datasrc type");
	memset(&req, 0, sizeof(req));
	req.htreq_server = server;
	req.htreq_port = strcmp(port, "") ? port : "http";

	req.htreq_method = "GET";
	req.htreq_version = "HTTP/1.1";
	req.htreq_url = path;
	return (http_open(&req, res));
}

/* HTTP data source flags. */
#define DHF_USEGSS	(1 << 0)

struct ustream *
ds_https(const char *server, const char *port, const char *path,
    struct http_res *res, int flags)
{
	struct ustream *usp;
	struct http_req req;

	if (path == NULL)
		errx(1, "no remote data available for datasrc type");
	memset(&req, 0, sizeof(req));
	req.htreq_server = server;
	req.htreq_port = strcmp(port, "") ? port : "https";

	req.htreq_method = "GET";
	req.htreq_version = "HTTP/1.1";
	req.htreq_url = path;

#ifdef _GSS
	if (flags & DHF_USEGSS)
		req.htreq_extraf = gss_build_auth;
	else
#endif
	{
		if ((req.htreq_extra = calloc(2, sizeof(char *))) == NULL)
			err(1, "calloc");
		if (asprintf(&req.htreq_extra[0],
		    "Authorization: Basic %s\r\n", login_auth) == -1)
			err(1, "asprintf");
	}

	usp = http_open(&req, res);

	if ((flags & DHF_USEGSS) == 0) {
		free(req.htreq_extra[0]);
		free(req.htreq_extra);
	}
	return (usp);
}

struct datasrc *
ds_open(int type)
{
	struct ustream *(*httpf)(const char *, const char *,
	    const char *, struct http_res *, int);
	char proto[MAX_PROTO_LEN];
	char host[MAXHOSTNAMELEN];
	char port[MAX_PORT_LEN];
	char path[MAX_PATH_LEN];
	struct http_res res;
	struct datasrc *ds;
	struct tm tm_zero;
	int ust, fd, flg;

	ds = &datasrcs[type];
	if (!uri_parse(ds->ds_uri, proto, host, port, path))
		return (NULL);
	if (strcmp(proto, "file") == 0) {
		ust = UST_LOCAL;
		if ((fd = open(path, O_RDONLY)) == -1) {
			strncat(path, ".gz",			/* XXX */
			    sizeof(path) - strlen(path) - 1);
			path[sizeof(path) - 1] = '\0';
			if ((fd = open(path, O_RDONLY)) == -1)
				return (NULL);
			ust = UST_GZIP;
		}
		ds->ds_us = us_init(fd, ust, "r");
	} else if (strcmp(proto, "http") == 0 ||
	    strcmp(proto, "https") == 0 ||
	    strcmp(proto, "gssapi") == 0) {
		flg = 0;

		httpf = ds_http;
		if (strcmp(proto, "gssapi") == 0) {
#ifdef _GSS
			if (gss_valid(host)) {
				httpf = ds_https;
				flg |= DHF_USEGSS;
			} else
#endif
			{
				dsfopts &= ~DSFO_SSL;
				snprintf(proto, sizeof(proto), "http");
				port[0] = '\0';
				ds_setlive();
				st.st_rf &= ~RF_DATASRC;
			}
		} else if (strcmp(proto, "https") == 0)
			httpf = ds_https;

		memset(&res, 0, sizeof(res));
		if ((ds->ds_us = httpf(host, port, path,
		    &res, flg)) == NULL)
			return (NULL);
		memset(&tm_zero, 0, sizeof(tm_zero));
		if (memcmp(&res.htres_mtime, &tm_zero,
		    sizeof(tm_zero)) != 0)
			ds->ds_mtime = mktime(&res.htres_mtime);
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
	} else {
		warnx("unsupported URI protocol");
	}
	return (ds);
}

/* Create a filename for a clone of a data set for a client. */
__inline void
dsc_fn(struct datasrc *ds, const char *sid, char *buf, size_t len)
{
	snprintf(buf, len, "%s/%s/%s", _PATH_SESSIONS, sid,
	    ds->ds_name);
}

/* Create a clone of a data set for a client. */
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
	struct stat stb;

	snprintf(fn, sizeof(fn), "%s/%s", _PATH_SESSIONS, sid);
	if (stat(fn, &stb) == -1) {
		/* XXX: more robust, check structure & dir */
		if (errno == ENOENT)
			return (0);
		else
			err(1, "stat");
	}
	return (1);
}

/* Clone a data set.  Create if needed. */
void
dsc_clone(int type, const char *sid)
{
	char buf[BUFSIZ], fn[BUFSIZ];
	struct timeval tv[2];
	struct datasrc *ds;
	struct stat stb;
	ssize_t n;
	int fd;

	if (!dsc_exists(sid))
		dsc_create(sid);

	ds = ds_open(type);
	/* XXX check for errors */
	dsc_fn(ds, sid, fn, sizeof(fn));
	if ((fd = open(fn, O_RDWR | O_CREAT, 0644)) == -1)
		err(1, "open: %s", fn);

	if (uri_has_proto(ds->ds_uri, "file")) {
		/* XXX: use us_stat */
		if (fstat(ds->ds_us->us_fd, &stb) == -1)
			err(1, "fstat");
	} else {
		stb.st_mtime = ds->ds_mtime;
	}

	/* XXXXXXXX: use us_read */
	while ((n = read(ds->ds_us->us_fd, &buf,
	    sizeof(buf))) != -1 && n != 0)
		if (write(fd, buf, (size_t)n) != n)
			err(1, "write");
	if (n == -1)
		err(1, "read");

	if (stb.st_mtime) {
		tv[0].tv_sec = stb.st_mtime;
		tv[0].tv_usec = 0L;
		tv[1] = tv[0];
		if (futimes(fd, tv) == -1)
			err(1, "futimes");
	}

	close(fd);
	ds_close(ds);
}

/* Open an existing cloned client data set. */
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
	/* XXX try .gz */
	ds->ds_us = us_init(fd, UST_LOCAL, "r");
	return (ds);
}

__inline void
dsc_close(struct datasrc *ds)
{
	us_close(ds->ds_us);
}

int
dsc_load(int type, const char *sid)
{
	struct datasrc *ds;

	ds = dsc_open(type, sid);
	if (ds == NULL) {
		warn("dsc_open(%s, %s)", datasrcs[type].ds_name, sid);
		return (0);
	}
	ds_read(ds);
	dsc_close(ds);
	return (1);
}

/* Map the state data mode to a data source type. */
int
st_dsmode(void)
{
	int ds = DS_INV;

	switch (st.st_dmode) {
	}
	return (ds);
}
