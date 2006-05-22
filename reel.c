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
#include "pathnames.h"
#include "reel.h"

int reelent_eq(const void *, const void *);

char reel_fn[PATH_MAX];
int  reel_len;
size_t reel_pos;

struct objlist reelent_list = { { NULL }, 0, 0, 0, 0, 10,
    sizeof(struct reel_ent), reelent_eq };
struct objlist reel_list = { { NULL }, 0, 0, 0, 0, 10,
    sizeof(struct reel), reel_eq };

int
reelent_eq(const void *elem, const void *arg)
{
	const struct reel_ent *re = elem;

	return (strcmp(re->re_name, arg) == 0);
}

int
reelent_cmp(const void *a, const void *b)
{
	const struct reel_ent **rea = (const struct reel_ent **)a;
	const struct reel_ent **reb = (const struct reel_ent **)b;

	return (strcmp((*rea)->re_name, (*reb)->re_name));
}

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
	struct reel_ent *re;
	DIR *dp;
	int n;

	reel_len = 0;
	if ((dp = opendir(reel_fn)) == NULL) {
		if (errno == ENOENT) {
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
				if (strcmp(dent->d_name, reel_fn) > 0)
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

		re = obj_get(dent->d_name, &reelent_list);
		strncpy(re->re_name, dent->d_name,
		    sizeof(re->re_name) - 1);
		re->re_name[sizeof(re->re_name) - 1] = '\0';
	}
	obj_batch_end(&reelent_list);
	closedir(dp);

	qsort(reelent_list.ol_reelents, reelent_list.ol_cur,
	    sizeof(struct reel_ent *), reelent_cmp);
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
	if (reel_pos >= reelent_list.ol_cur)
		return;

	snprintf(fn_node, sizeof(fn_node), "%s/%s/node",
	    reel_fn, reelent_list.ol_reelents[reel_pos]->re_name);
	snprintf(fn_job, sizeof(fn_job), "%s/%s/job",
	    reel_fn, reelent_list.ol_reelents[reel_pos]->re_name);
	snprintf(fn_yod, sizeof(fn_yod), "%s/%s/yod",
	    reel_fn, reelent_list.ol_reelents[reel_pos]->re_name);

	datasrcs[DS_NODE].ds_flags |= DSF_FORCE;
	datasrcs[DS_JOB].ds_flags |= DSF_FORCE;
	datasrcs[DS_YOD].ds_flags |= DSF_FORCE;

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
