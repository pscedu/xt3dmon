/* $Id$ */

#include <sys/types.h>

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

struct datasrc {
	const char	*ds_lpath;
	const char	*ds_rpath;
	void		(*ds_parsef)(int *);
	void		(*ds_dbf)(void);
} datasrcs[] = {
	{ _PATH_TEMPMAP,  _RPATH_TEMP,  parse_tempmap,	db_tempmap },
	{ _PATH_PHYSMAP,  _RPATH_PHYS,  parse_physmap,	db_physmap },
	{ _PATH_JOBMAP,   _RPATH_JOBS,  parse_jobmap,	db_jobmap },
	{ _PATH_BADMAP,   _RPATH_BAD,   parse_badmap,	db_badmap },
	{ _PATH_CHECKMAP, _RPATH_CHECK, parse_checkmap,	db_checkmap },
	{ _PATH_QSTAT,    _RPATH_QSTAT, parse_qstat,	db_qstat },
	{ _PATH_STAT,     _PATH_STAT,   parse_mem,	NULL },
	{ _PATH_FAILMAP,  _RPATH_FAIL,  parse_failmap,	db_failmap }
};
#define NDATASRCS (sizeof(datasrcs) / sizeof(datasrcs[0]))

void
ds_refresh(int type, int flags)
{
	struct datasrc *ds;
	int fd;

	ds = &datasrcs[type];
	switch (dsp) {
	case DSP_LOCAL:
		fd = open(ds->ds_lpath, O_RDONLY);
		goto parse;
	case DSP_REMOTE:
		fd = ds_http(ds->ds_rpath);
parse:
		if (fd == -1 && (flags & DSF_CRIT) == 0) {
			warn("datasrc (%d) open failed", type);
			break;
		}
		ds->ds_parsef(&fd);
		if (fd != -1)
			close(fd);
		break;
	case DSP_DB:
		ds->ds_dbf();
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
