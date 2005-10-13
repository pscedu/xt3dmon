/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <fcntl.h>

#include "mon.h"

#define _RPATH_TEMP	"/xt3data/temps"
#define _RPATH_PHYS	"/xt3data/rtrtrace"
#define _RPATH_JOBS	"/xt3data/nids_list_phantom"
#define _RPATH_BAD	"/xt3data/bad_nids_list_phantom"
#define _RPATH_CHECK	"/xt3data/to_check_nids_list_phantom"
#define _RPATH_QSTAT	"/xt3data/qstat.out"
#define _RPATH_FAIL	"/xt3data/fails"

#define RDS_HOST	"mugatu"
#define RDS_PORT	80

int ds_http(const char *);

#define DSF_AUTO	(1<<0)
#define DSF_FORCE	(1<<1)
#define DSF_READY	(1<<2)

struct datasrc {
	time_t		  ds_mtime;
	int		  ds_flags;
	int		  ds_dsp;
	const char	 *ds_lpath;
	const char	 *ds_rpath;
	void		(*ds_parsef)(int *);
	void		(*ds_dbf)(void);
	struct objlist	 *ds_objlist;
} datasrcs[] = {
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_TEMPMAP,  _RPATH_TEMP,  parse_tempmap,	db_tempmap,	&temp_list },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_PHYSMAP,  _RPATH_PHYS,  parse_physmap,	db_physmap,	NULL },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_JOBMAP,   _RPATH_JOBS,  parse_jobmap,	db_jobmap,	&job_list },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_BADMAP,   _RPATH_BAD,   parse_badmap,	db_badmap,	NULL },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_CHECKMAP, _RPATH_CHECK, parse_checkmap,	db_checkmap,	NULL },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_QSTAT,    _RPATH_QSTAT, parse_qstat,	db_qstat,	NULL },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_STAT,     NULL,		parse_mem,	NULL,		NULL },
	{ 0, DSF_AUTO, DSP_LOCAL, _PATH_FAILMAP,  _RPATH_FAIL,  parse_failmap,	db_failmap,	&fail_list }
};
#define NDATASRCS (sizeof(datasrcs) / sizeof(datasrcs[0]))

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
	int fd, mod;

	mod = 0;
	ds = &datasrcs[type];
	switch (ds->ds_dsp) {
	case DSP_LOCAL:
		fd = open(ds->ds_lpath, O_RDONLY);
		if (fd == -1)
			goto parse;
		if (fstat(fd, &st) == -1)
			err(1, "fstat %s", ds->ds_lpath);
		/* XXX: no way to tell if it was modified with <1 second resolution. */
		if (st.st_mtime <= ds->ds_mtime)
			goto closeit;
		ds->ds_mtime = st.st_mtime;
		if ((ds->ds_flags & (DSF_AUTO | DSF_FORCE)) == 0) {
//			ds->ds_flags |= DSF_READY;
			status_add("New data available");
			goto closeit;
		}
		ds->ds_flags &= ~DSF_FORCE;
		goto parse;
	case DSP_REMOTE:
		fd = ds_http(ds->ds_rpath);
parse:
		if (fd == -1 && (flags & DSF_CRIT) == 0) {
			if ((flags & DSF_IGN) == 0)
				warn("datasrc (%d) open failed", type);
			break;
		}
		if (ds->ds_objlist)
			obj_batch_start(ds->ds_objlist);
		ds->ds_parsef(&fd);
		if (ds->ds_objlist)
			obj_batch_end(ds->ds_objlist);
closeit:
		if (fd != -1)
			close(fd);
		break;
	case DSP_DB:
		if (ds->ds_objlist)
			obj_batch_start(ds->ds_objlist);
		ds->ds_dbf();
		if (ds->ds_objlist)
			obj_batch_end(ds->ds_objlist);
		break;
	}
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
