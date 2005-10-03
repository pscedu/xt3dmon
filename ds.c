/* $Id$ */

#define DS_TEMP		0
#define DS_PHYS		1
#define DS_JOBS		2
#define DS_BAD		3
#define DS_CHECK	4
#define DS_QSTAT	5
#define DS_MEM		6
#define DS_FAIL		7

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
	const char *ds_lpath;
	const char *ds_rpath;
} datasrcs[] = {
	{ _PATH_TEMPMAP,	_RPATH_TEMP },
	{ _PATH_PHYSMAP,	_RPATH_PHYS },
	{ _PATH_JOBMAP,		_RPATH_JOBS },
	{ _PATH_BADMAP,		_RPATH_BAD },
	{ _PATH_CHECKMAP,	_RPATH_CHECK },
	{ _PATH_QSTAT,		_RPATH_QSTAT },
	{ _PATH_STAT,		_RPATH_STAT },
	{ _PATH_FAILMAP,	_RPATH_FAIL }
};
#define NDATASRCS (sizeof(datasrcs) / sizeof(datasrcs[0]))

#define DSF_CRIT	(1<<0)
#define DSF_REMOTE	(1<<1)

int
ds_open(int type, int flags)
{
	struct datasrc *ds;

	if (type < 0 || type >= NDATASRCS)
		errx(1, "datasrc type out of range");
	ds = datasrcs[type];

	if (flags & DSF_REMOTE) {
		struct http_req r;

		if (ds->ds_rpath == NULL)
			errx(1, "no remote data available for datasrc type");
		memset(&r, 0, sizeof(r));
		r->htreq_server = RDS_HOST;
		r->htreq_port = RDS_PORT;

		r->htreq_method = "GET";
		r->htreq_version = "HTTP/1.1";
		r->htreq_version = ds->ds_rpath;
		return (http_open(&r));
	}
	if ((fd = open(ds->ds_lpath, O_RDONLY)) == -1) {
		if (flags & DSF_CRIT)
			err(1, "%s", ds->ds_lpath);
		warn(1, "%s", ds->ds_lpath);
		return (-1);
	}
	return (fd);
}
