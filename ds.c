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

struct datasrc {
	const char	*ds_lpath;
	const char	*ds_rpath;
	void		(*ds_dbf)(void);
} datasrcs[] = {
	{ _PATH_TEMPMAP,  _RPATH_TEMP,  db_tempmap },
	{ _PATH_PHYSMAP,  _RPATH_PHYS,  db_physmap },
	{ _PATH_JOBMAP,   _RPATH_JOBS,  db_jobmap },
	{ _PATH_BADMAP,   _RPATH_BAD,   db_badmap },
	{ _PATH_CHECKMAP, _RPATH_CHECK, db_checkmap },
	{ _PATH_QSTAT,    _RPATH_QSTAT, db_qstat },
	{ _PATH_STAT,     _PATH_STAT,   NULL },
	{ _PATH_FAILMAP,  _RPATH_FAIL,  db_failmap }
};
#define NDATASRCS (sizeof(datasrcs) / sizeof(datasrcs[0]))

void
ds_refresh(int type, int flags)
{
	switch (dsp) {
	case DSP_LOCAL:
	case DSP_REMOTE:
		ds_open(type, flags);
		break;
	case DSP_DB:
		break;
	}
}

int
ds_open(int type, int flags)
{
	struct datasrc *ds;
	int fd;

	if (type < 0 || type >= (int)NDATASRCS)
		errx(1, "datasrc type out of range");
	ds = &datasrcs[type];

	switch (dsp) {
	case DSP_REMOTE: {
		struct http_req r;

		if (ds->ds_rpath == NULL)
			errx(1, "no remote data available for datasrc type");
		memset(&r, 0, sizeof(r));
		r.htreq_server = RDS_HOST;
		r.htreq_port = RDS_PORT;

		r.htreq_method = "GET";
		r.htreq_version = "HTTP/1.1";
		r.htreq_version = ds->ds_rpath;
		fd = http_open(&r, NULL);
		break;
	    }
	case DSP_LOCAL:
		if ((fd = open(ds->ds_lpath, O_RDONLY)) == -1) {
			if (flags & DSF_CRIT)
				err(1, "%s", ds->ds_lpath);
			warn("%s", ds->ds_lpath);
			return (-1);
		}
		break;
	}
	return (fd);
}
