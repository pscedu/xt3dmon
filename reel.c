/* $Id$ */

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

char reel_fn[PATH_MAX];
int  reel_len;
size_t reel_pos;

struct objlist reelent_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct fnent), fe_eq };
struct objlist reel_list = { NULL, 0, 0, 0, 0, 10, sizeof(struct reel), reel_eq };

int
reel_eq(const void *elem, const void *arg)
{
	const struct reel *rl = elem;

	return (strcmp(rl->rl_name, arg) == 0);
}

int
reel_cmp(const void *a, const void *b)
{
	const struct reel **rla = (const struct reel **)a;
	const struct reel **rlb = (const struct reel **)b;

	return (strcmp((*rla)->rl_name, (*rlb)->rl_name));
}

void
reel_load(void)
{
	struct dirent *dent;
	struct fnent *re;
	char *rfn;
	DIR *dp;
	int n;

	reel_len = 0;
	if ((dp = opendir(reel_fn)) == NULL) {
		if (errno == ENOENT ||
		    errno == EINVAL) {
			reel_fn[0] = '\0';
			/* User hasn't specified, so grab newest reel. */
			if ((dp = opendir(_PATH_ARCHIVE)) == NULL) {
				/*
				 * It's OK if archive dir
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

				if ((rfn = strrchr(reel_fn, '/')) == NULL ||
				    strcmp(dent->d_name, rfn + 1) > 0)
					snprintf(reel_fn,
					    sizeof(reel_fn), "%s/%s",
					    _PATH_ARCHIVE, dent->d_name);
			}
			/* XXX: check for error */
			closedir(dp);
		} else
			err(1, "opendir %s", reel_fn);

		if (reel_fn[0] == '\0')
			return;

		if ((dp = opendir(reel_fn)) == NULL)
			err(1, "opendir %s", reel_fn);
	}
	obj_batch_start(&reelent_list);
	for (n = 0; (dent = readdir(dp)) != NULL; n++) {
		if (strcmp(dent->d_name, ".") == 0 ||
		    strcmp(dent->d_name, "..") == 0 ||
		    strcmp(dent->d_name, "last") == 0)
			continue;

		/* XXX: check stat and ISFILE */

		re = obj_get(dent->d_name, &reelent_list);
		strncpy(re->fe_name, dent->d_name,
		    sizeof(re->fe_name) - 1);
		re->fe_name[sizeof(re->fe_name) - 1] = '\0';
	}
	obj_batch_end(&reelent_list);
	/* XXX: check for error */
	closedir(dp);

	qsort(reelent_list.ol_data, reelent_list.ol_cur,
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

	if (reel_pos >= reelent_list.ol_cur)
		return;

	snprintf(fn_node, sizeof(fn_node), "%s/%s/node",
	    reel_fn, OLE(reelent_list, reel_pos, fnent)->fe_name);
	snprintf(fn_job, sizeof(fn_job), "%s/%s/job",
	    reel_fn, OLE(reelent_list, reel_pos, fnent)->fe_name);
	snprintf(fn_yod, sizeof(fn_yod), "%s/%s/yod",
	    reel_fn, OLE(reelent_list, reel_pos, fnent)->fe_name);

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
