/* $Id$ */

/*
 * Reel routines.
 *
 * Archived data is shown in a "film reel" fashion.
 * reel_list maintains a list of directories/files used
 * for reel selection and reel_frame_list keeps a sorted
 * list of the subdirectories from the chosen reel.
 */

#include "mon.h"

#include <sys/stat.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ds.h"
#include "objlist.h"
#include "panel.h"
#include "pathnames.h"
#include "reel.h"
#include "state.h"

char	reel_dir[PATH_MAX];
char	reel_browsedir[PATH_MAX] = _PATH_ARCHIVE;
size_t	reel_pos;

struct objlist reelframe_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };
struct objlist reel_list      = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };

void
reel_load(void)
{
	char path[PATH_MAX];
	struct dirent *dent;
	struct fnent *fe;
	struct stat stb;
	char *rfn;
	DIR *dp;
	int n;

	if ((dp = opendir(reel_dir)) == NULL) {
		if (errno == ENOENT ||
		    errno == EINVAL) {
			reel_dir[0] = '\0';
			/* User hasn't specified, so grab newest reel. */
			if ((dp = opendir(_PATH_ARCHIVE)) == NULL) {
				/*
				 * Don't die if archive directory
				 * doesn't exist at all.
				 */
				if (errno != ENOENT)
					warn("opendir %s", _PATH_ARCHIVE);
				return;
			}
			while ((dent = readdir(dp)) != NULL) {
				if (strcmp(dent->d_name, ".") == 0 ||
				    strcmp(dent->d_name, "..") == 0)
					continue;
				/* XXX: check stat and ISDIR */

				if ((rfn = strrchr(reel_dir, '/')) == NULL ||
				    strcmp(dent->d_name, rfn + 1) > 0)
					snprintf(reel_dir,
					    sizeof(reel_dir), "%s/%s",
					    _PATH_ARCHIVE, dent->d_name);
			}
			/* XXX: check for error */
			closedir(dp);
		} else
			err(1, "opendir %s", reel_dir);

		if (reel_dir[0] == '\0')
			return;

		if ((dp = opendir(reel_dir)) == NULL)
			err(1, "opendir %s", reel_dir);
	}
	obj_batch_start(&reelframe_list);
	for (n = 0; (dent = readdir(dp)) != NULL; n++) {
		if (dent->d_name[0] == '.')
			continue;

		snprintf(path, sizeof(path), "%s/%s",
		    reel_dir, dent->d_name);
		if (stat(path, &stb) == -1)
			err(1, "stat %s", path);
		if (!S_ISDIR(stb.st_mode))
			continue;

		fe = obj_get(dent->d_name, &reelframe_list);
		snprintf(fe->fe_name, sizeof(fe->fe_name), "%s",
		    dent->d_name);
	}
	obj_batch_end(&reelframe_list);
	/* XXX: check for error */
	closedir(dp);

	qsort(reelframe_list.ol_data, reelframe_list.ol_cur,
	    sizeof(struct fnent *), fe_cmp);
}

/* XXX: bad */
int dsp_node;
int dsp_job;
int dsp_yod;

char fn_node[PATH_MAX];
char fn_job[PATH_MAX];
char fn_yod[PATH_MAX];

void
reel_start(void)
{
	dsp_node = datasrcs[DS_NODE].ds_dsp;
	dsp_job = datasrcs[DS_JOB].ds_dsp;
	dsp_yod = datasrcs[DS_YOD].ds_dsp;

	datasrcs[DS_NODE].ds_dsp = DSP_LOCAL;
	datasrcs[DS_JOB].ds_dsp = DSP_LOCAL;
	datasrcs[DS_YOD].ds_dsp = DSP_LOCAL;

	datasrcs[DS_NODE].ds_lpath = fn_node;
	datasrcs[DS_JOB].ds_lpath = fn_job;
	datasrcs[DS_YOD].ds_lpath = fn_yod;

	reel_pos = 0;
	reel_advance();
}

void
reel_advance(void)
{
	struct panel *p;

	if (reel_pos >= reelframe_list.ol_cur)
		return;

	snprintf(fn_node, sizeof(fn_node), "%s/%s/node",
	    reel_dir, OLE(reelframe_list, reel_pos, fnent)->fe_name);
	snprintf(fn_job, sizeof(fn_job), "%s/%s/job",
	    reel_dir, OLE(reelframe_list, reel_pos, fnent)->fe_name);
	snprintf(fn_yod, sizeof(fn_yod), "%s/%s/yod",
	    reel_dir, OLE(reelframe_list, reel_pos, fnent)->fe_name);

	st.st_rf |= RF_DATASRC;
	datasrcs[DS_NODE].ds_flags |= DSF_FORCE;
	datasrcs[DS_JOB].ds_flags |= DSF_FORCE;
	datasrcs[DS_YOD].ds_flags |= DSF_FORCE;

	if ((p = panel_for_id(PANEL_REEL)) != NULL)
		p->p_opts |= POPT_REFRESH;

	reel_pos++;
}

void
reel_end(void)
{
	st.st_rf |= RF_DATASRC;
	datasrcs[DS_NODE].ds_dsp = dsp_node;
	datasrcs[DS_JOB].ds_dsp = dsp_job;
	datasrcs[DS_YOD].ds_dsp = dsp_yod;

	datasrcs[DS_NODE].ds_lpath = _PATH_NODE;
	datasrcs[DS_JOB].ds_lpath = _PATH_JOB;
	datasrcs[DS_YOD].ds_lpath = _PATH_YOD;

	datasrcs[DS_NODE].ds_flags |= DSF_FORCE;
	datasrcs[DS_JOB].ds_flags |= DSF_FORCE;
	datasrcs[DS_YOD].ds_flags |= DSF_FORCE;
}

char *
reel_set(const char *fn, int flags)
{
	char path[PATH_MAX];
	struct stat stb;
	struct panel *p;

	if ((flags & CHF_DIR) == 0)
		errx(1, "file chosen when directory-only");

	if ((p = panel_for_id(PANEL_REEL)) != NULL)
		p->p_opts |= POPT_REFRESH;

	snprintf(path, sizeof(path), "%s/%s/%s",
	    reel_browsedir, fn, _PATH_ISAR);
	if (stat(path, &stb) == -1) {
		if (errno != ENOENT)
			err(1, "stat %s", path);
		errno = 0;
	} else {
		/* Archive index exists, don't descend. */
		if (strcmp(fn, "..") != 0) {
			snprintf(reel_dir, sizeof(reel_dir),
			    "%s/%s", reel_browsedir, fn);
			return (NULL);
		}
	}
	return (reel_browsedir);
}
