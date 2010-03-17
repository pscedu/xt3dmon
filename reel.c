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

#include "buf.h"
#include "ds.h"
#include "objlist.h"
#include "panel.h"
#include "pathnames.h"
#include "reel.h"
#include "state.h"
#include "util.h"

char	reel_dir[PATH_MAX] = LATEST_REEL;
char	reel_browsedir[PATH_MAX] = _PATH_ARCHIVE;
size_t	reel_pos = -1;

struct objlist reelframe_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };
struct objlist reel_list      = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };

void
reel_load(void)
{
	char path[PATH_MAX];
	struct dirent *dent;
	struct fnent *fe;
	struct stat stb;
	DIR *dp;
	int n;

	if ((dp = opendir(reel_dir)) == NULL) {
		warn("opendir %s", reel_dir);
		return;
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

void
reel_start(void)
{
	reel_pos = -1;
	reel_advance();
}

void
reel_advance(void)
{
	struct buf uri;
	int j;

	if (reel_pos + 1 >= reelframe_list.ol_cur)
		return;
	reel_pos++;

	buf_init(&uri);
	buf_appendv(&uri, "file://");
	escape_printf(&uri, reel_dir);
	buf_append(&uri, '/');
	escape_printf(&uri, OLE(reelframe_list, reel_pos, fnent)->fe_name);
	buf_appendv(&uri, "/%s");
	buf_nul(&uri);
	for (j = 0; j < NDS; j++)
		if (datasrcs[j].ds_flags & DSF_LIVE)
			ds_seturi(j, buf_get(&uri));
	buf_free(&uri);
	panel_rebuild(PANEL_REEL);
}

void
reel_end(void)
{
	/*
	 * XXX restore to last dataset, which may
	 * not be possible if it was a URI.
	 */
	ds_setlive();
	panel_rebuild(PANEL_REEL);
}

char *
reel_set(const char *fn, int flags)
{
	char path[PATH_MAX];
	struct stat stb;

	if ((flags & CHF_DIR) == 0)
		errx(1, "file chosen when directory-only");

	panel_rebuild(PANEL_REEL);
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
			reel_load();
			return (NULL);
		}
	}
	return (reel_browsedir);
}
