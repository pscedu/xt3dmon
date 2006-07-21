/* $Id$ */

#include "mon.h"

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cdefs.h"
#include "buf.h"
#include "deusex.h"
#include "ds.h"
#include "env.h"
#include "fill.h"
#include "flyby.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "pathnames.h"
#include "queue.h"
#include "reel.h"
#include "select.h"
#include "selnode.h"
#include "server.h"
#include "state.h"
#include "uinp.h"
#include "util.h"
#include "yod.h"

int	 dmode_data_clean;
int	 selnode_clean;
int	 hlnc_clean;

/* Blink panel text (swap between two strings during interval). */
int
panel_blink(struct timeval *pre, char **s, int size, int *i, long interval)
{
	struct timeval tv, diff;
	char *str = s[*i];
	int tog = 0;

	gettimeofday(&tv, NULL);
	timersub(&tv, pre, &diff);

	/* Make it blink once per second */
	if (diff.tv_sec * 1e6 + diff.tv_usec > interval) {
		*pre = tv;

		/* Swap */
		if(++(*i) == size)
			*i = 0;
		str = s[*i];
		tog = 1;
	}

	return tog;
}

void
panel_set_content(struct panel *p, char *fmt, ...)
{
	struct panel *t;
	va_list ap;
	size_t len;

	len = 0; /* gcc */
	p->p_opts |= POPT_DIRTY;
	/* XXX: use asprintf */
	for (;;) {
		va_start(ap, fmt);
		len = vsnprintf(p->p_str, p->p_strlen,
		    fmt, ap);
		va_end(ap);
		if (len >= p->p_strlen) {
			p->p_strlen = len + 1;
			if ((p->p_str = realloc(p->p_str,
			    p->p_strlen)) == NULL)
				err(1, "realloc");
		} else
			break;
	}
	if (p->p_str[len - 1] == '\n')
		p->p_str[len - 1] = '\0';
	/* All panels below us must be refreshed now, too. */
	for (t = p; t != TAILQ_END(&panels); t = TAILQ_NEXT(t, p_link))
		t->p_opts |= POPT_DIRTY;
}

void
panel_add_content(struct panel *p, char *fmt, ...)
{
	char *oldstr, *newstr;
	va_list ap;

	if ((oldstr = strdup(p->p_str)) == NULL)
		err(1, "strdup");

	va_start(ap, fmt);
	if (vasprintf(&newstr, fmt, ap) == -1)
		err(1, "vasprintf");
	va_end(ap);

	panel_set_content(p, "%s%s", oldstr, newstr);

	free(newstr);
	free(oldstr);
}

__inline int
panel_ready(struct panel *p)
{
	return ((p->p_opts & POPT_REFRESH) == 0 && p->p_str != NULL);
}

void
pwidget_startlist(struct panel *p)
{
	p->p_nwidgets = 0;
	p->p_maxwlen = 0;
	p->p_nextwidget = &SLIST_FIRST(&p->p_widgets);
}

void
pwidget_endlist(struct panel *p)
{
	struct pwidget *pw, *nextp;

	if (p->p_nwidgets == 0)
		SLIST_INIT(&p->p_widgets);

	if (*p->p_nextwidget == NULL)
		return;
	pw = *p->p_nextwidget;
	*p->p_nextwidget = NULL;

	for (; pw; pw = nextp) {
		nextp = SLIST_NEXT(pw, pw_next);
		free(pw);
	}
}

void
pwidget_add(struct panel *p, struct fill *fp, const char *s,
    void (*cb)(struct glname *, int), int cbarg)
{
	struct pwidget *pw;

	p->p_nwidgets++;
	if (*p->p_nextwidget == NULL) {
		if ((pw = malloc(sizeof(*pw))) == NULL)
			err(1, "malloc");
		SLIST_NEXT(pw, pw_next) = NULL;
		*p->p_nextwidget = pw;
	} else
		pw = *p->p_nextwidget;

	p->p_nextwidget = &SLIST_NEXT(pw, pw_next);

	pw->pw_fillp = fp;
	pw->pw_cb = cb;
	pw->pw_cbarg = cbarg;
	pw->pw_str = s;
	p->p_maxwlen = MAX(p->p_maxwlen, strlen(s));
}

void
panel_refresh_fps(struct panel *p)
{
	static long ofps = -1;

	if (ofps == fps && panel_ready(p))
		return;
	ofps = fps;
	panel_set_content(p, "FPS: %d", fps);
}

void
panel_refresh_cmd(struct panel *p)
{
	/* XXX:  is the mode_data_clean check correct? */
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 &&
	    selnode_clean & PANEL_CMD &&
	    panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;
	selnode_clean |= PANEL_CMD;

	if (SLIST_EMPTY(&selnodes))
		panel_set_content(p,
		    "- Send Command to Node -\n"
		    "No node(s) selected.");
	else
		panel_set_content(p,
		    "- Send Command to Node -\n"
		    "nid%d> %s",
		    SLIST_FIRST(&selnodes)->sn_nodep->n_nid,
		    buf_get(&uinp.uinp_buf));
}

void
panel_refresh_legend(struct panel *p)
{
	size_t j;
	int i;

	if (dmode_data_clean & PANEL_LEGEND &&
	    hlnc_clean & PANEL_LEGEND && panel_ready(p))
		return;
	dmode_data_clean |= PANEL_LEGEND;
	hlnc_clean |= PANEL_LEGEND;

	pwidget_startlist(p);
	switch (st.st_dmode) {
	case DM_JOB:
		panel_set_content(p, "- Job Legend -\n"
		    "Total jobs: %lu", job_list.ol_cur);

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);

		for (j = 0; j < NSC; j++) {
			if (j == SC_USED ||
			    statusclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &statusclass[j].nc_fill,
			    statusclass[j].nc_name, gscb_pw_hlnc, j);
		}
		for (j = 0; j < job_list.ol_cur; j++)
			pwidget_add(p, &job_list.ol_jobs[j]->j_fill,
			    job_list.ol_jobs[j]->j_name, gscb_pw_hlnc, NSC + j);
		break;
	case DM_FAIL:
		panel_set_content(p, "- Failure Legend -\n"
		    "Total: %lu", total_failures);

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);
		pwidget_add(p, &fill_nodata, "No data", NULL, 0);

		for (j = 0; j < NFAILC; j++) {
			if (failclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &failclass[j].nc_fill,
			    failclass[j].nc_name, gscb_pw_hlnc, j);
		}
		break;
	case DM_TEMP:
		panel_set_content(p, "- Temperature Legend -");

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);
		pwidget_add(p, &fill_nodata, "No data", NULL, 0);

		for (i = NTEMPC - 1; i >= 0; i--) {
			if (tempclass[i].nc_nmemb == 0)
				continue;
			pwidget_add(p, &tempclass[i].nc_fill,
			    tempclass[i].nc_name, gscb_pw_hlnc, i);
		}
		break;
	case DM_YOD:
		panel_set_content(p, "- Yod Legend -\n"
		    "Total yods: %lu", yod_list.ol_cur);

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);

		for (j = 0; j < NSC; j++) {
			if (j == SC_USED ||
			    statusclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &statusclass[j].nc_fill,
			    statusclass[j].nc_name, gscb_pw_hlnc, j);
		}
		for (j = 0; j < yod_list.ol_cur; j++)
			pwidget_add(p, &yod_list.ol_yods[j]->y_fill,
			    yod_list.ol_yods[j]->y_cmd, gscb_pw_hlnc, NSC + j);
		break;
	case DM_RTUNK:
		panel_set_content(p, "- Routing Legend -");

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);
		pwidget_add(p, &fill_rtesnd, "Sender", gscb_pw_hlnc, RTC_SND);
		pwidget_add(p, &fill_rtercv, "Target", gscb_pw_hlnc, RTC_RCV);

		for (j = 0; j < NRTC; j++) {
			if (rtclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &rtclass[j].nc_fill,
			    rtclass[j].nc_name, gscb_pw_hlnc, j);
		}
		break;
	case DM_SEASTAR:
		panel_set_content(p, "- Seastar Legend -");

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);

		for (j = 0; j < NSSC; j++) {
			if (ssclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &ssclass[j].nc_fill,
			    ssclass[j].nc_name, gscb_pw_hlnc, j);
		}
		break;
	case DM_SAME:
		panel_set_content(p, "- Legend -");

		pwidget_add(p, &fill_same, "All nodes", gscb_pw_hlnc, HL_ALL);
		break;
	case DM_LUSTRE:
		panel_set_content(p, "- Lustre Legend -");

		pwidget_add(p, &fill_showall, "Show all", gscb_pw_hlnc, HL_ALL);

		for (j = 0; j < NLUSTC; j++) {
			if (lustreclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &lustreclass[j].nc_fill,
			    lustreclass[j].nc_name, gscb_pw_hlnc, j);
		}
		break;
	default:
		panel_set_content(p, "- Legend -\n"
		    "Not available.");
		break;
	}
	pwidget_endlist(p);
}

#define NINFO_MAXNIDS 30

void
panel_refresh_ninfo(struct panel *p)
{
	struct physcoord pc;
	struct objhdr *ohp;
	struct objlist *ol;
	struct selnode *sn;
	struct ivec *iv;
	struct node *n;
	int j;

	/*
	 * Since dmode-dependent data is written into
	 * the panel content, if the dmode changes, so
	 * must the panel content.
	 */
	if (selnode_clean & PANEL_NINFO && (SLIST_EMPTY(&selnodes) ||
	    (dmode_data_clean & PANEL_NINFO)) && panel_ready(p))
		return;
	dmode_data_clean |= PANEL_NINFO;
	selnode_clean |= PANEL_NINFO;

	if (SLIST_EMPTY(&selnodes)) {
		panel_set_content(p, "- Node Information -\n"
		    "No node(s) selected");
		return;
	}

	if (nselnodes > 1) {
		struct buf b_nids, bw_nids, b_data, bw_data;
		size_t j;

		buf_init(&b_nids);
		buf_init(&b_data);

		buf_appendv(&b_nids, "Node ID(s): ");

		ol = NULL; /* gcc */
		switch (st.st_dmode) {
		case DM_JOB:
			buf_appendv(&b_data, "Job ID(s): ");
			ol = &job_list;
			break;
		case DM_YOD:
			buf_appendv(&b_data, "Yod ID(s): ");
			ol = &yod_list;
			break;
		}

		j = 0;
		SLIST_FOREACH(sn, &selnodes, sn_next) {
			n = sn->sn_nodep;

			if (j == NINFO_MAXNIDS)
				buf_appendv(&b_nids, "...."); /* chop() */
			else if (j < NINFO_MAXNIDS)
				buf_appendfv(&b_nids, "%d,", n->n_nid);

			switch (st.st_dmode) {
			case DM_JOB:
				if (n->n_job)
					n->n_job->j_oh.oh_flags |= OHF_TMP;
				break;
			case DM_YOD:
				if (n->n_yod)
					n->n_yod->y_oh.oh_flags |= OHF_TMP;
				break;
			}
			j++;
		}
		buf_chop(&b_nids);
		buf_append(&b_nids, '\0');
		text_wrap(&bw_nids, buf_get(&b_nids), 25, "\n  ", 2);
		buf_free(&b_nids);

		for (j = 0; ol && j < ol->ol_cur; j++) {
			ohp = ol->ol_data[j];
			if (ohp->oh_flags & OHF_TMP) {
				ohp->oh_flags &= ~OHF_TMP;
				switch (st.st_dmode) {
				case DM_JOB:
					buf_appendfv(&b_data, "%d,",
					    ((struct job *)ohp)->j_id);
					break;
				case DM_YOD:
					buf_appendfv(&b_data, "%d,",
					    ((struct yod *)ohp)->y_id);
					break;
				}
			}
		}
		buf_chop(&b_data);
		buf_append(&b_data, '\0');
		text_wrap(&bw_data, buf_get(&b_data), 25, "\n  ", 2);
		buf_free(&b_data);

		panel_set_content(p,
		    "- Node Information -\n"
		    "%d node(s) selected\n"
		    "%s",
		    nselnodes, buf_get(&bw_nids));
		if (j)
			panel_add_content(p, "\n%s",
			    buf_get(&bw_data));

		buf_free(&bw_nids);
		buf_free(&bw_data);
		return;
	}

	/* Only one selected node. */
	n = SLIST_FIRST(&selnodes)->sn_nodep;
	node_physpos(n, &pc);
	iv = &n->n_wiv;

	panel_set_content(p,
	    "- Node Information -\n"
	    "Node ID: %d\n"
	    "Wired position: (%d,%d,%d)\n"
	    "Hardware name: c%d-%dc%ds%dn%d\n"
	    "Status: %s",
	    n->n_nid,
	    iv->iv_x, iv->iv_y, iv->iv_z,
	    pc.pc_cb, pc.pc_r, pc.pc_cg, pc.pc_m, pc.pc_n,
	    statusclass[n->n_state].nc_name);

	if (n->n_temp == DV_NODATA)
		panel_add_content(p, "\nTemperature: N/A");
	else
		panel_add_content(p, "\nTemperature: %dC", n->n_temp);

	if (memcmp(&rt_max, &rt_zero, sizeof(rt_zero)) != 0) {
		panel_add_content(p, "\n\nRouting errors:");
		if (memcmp(&n->n_route.rt_err, &rt_zero, sizeof(rt_zero)) == 0)
			panel_add_content(p, " none");
		else {
			for (j = 0; j < NRP; j++) {
				if (n->n_route.rt_err[j][RT_RECOVER] ||
				    n->n_route.rt_err[j][RT_FATAL] ||
				    n->n_route.rt_err[j][RT_ROUTER]) {
					int needcomma = 0;

					panel_add_content(p, "\n  port %d: ", j);
					if (n->n_route.rt_err[j][RT_RECOVER]) {
						panel_add_content(p, "%d recover",
						    n->n_route.rt_err[j][RT_RECOVER]);
						needcomma = 1;
					}
					if (n->n_route.rt_err[j][RT_FATAL]) {
						panel_add_content(p, "%s%d fatal",
						    needcomma ? ", " : "",
						    n->n_route.rt_err[j][RT_FATAL]);
					needcomma = 1;
					}
					if (n->n_route.rt_err[j][RT_ROUTER]) {
						panel_add_content(p, "%s%d rtr",
						    needcomma ? ", " : "",
						    n->n_route.rt_err[j][RT_ROUTER]);
					}
				}
			}
		}
	}

	if (n->n_job) {
		panel_add_content(p, "\n\nJob ID: %d", n->n_job->j_id);
		if (strcmp(n->n_job->j_owner, DV_NOAUTH) != 0)
			panel_add_content(p, "\nJob owner: %s", n->n_job->j_owner);
		if (strcmp(n->n_job->j_name, DV_NOAUTH) != 0)
			panel_add_content(p, "\nJob name: %s", n->n_job->j_name);
		if (strcmp(n->n_job->j_queue, DV_NOAUTH) != 0)
			panel_add_content(p, "\nJob queue: %s", n->n_job->j_queue);
		panel_add_content(p,
		    "\n"
		    "Job duration: %d:%02d\n"
		    "Job time used: %d:%02d (%d%%)\n"
		    "Job # of CPUs: %d",
		    n->n_job->j_tmdur / 60,
		    n->n_job->j_tmdur % 60,
		    n->n_job->j_tmuse / 60,
		    n->n_job->j_tmuse % 60,
		    n->n_job->j_tmuse * 100 /
		      (n->n_job->j_tmdur ?
		       n->n_job->j_tmdur : 1),
		    n->n_job->j_ncpus);
		if (n->n_job->j_mem)
			panel_add_content(p,
			    "\nJob Login Node Memory: %dKB",
			    n->n_job->j_mem);
	}

	if (n->n_yod) {
		panel_add_content(p,
		    "\n\n"
		    "Yod ID: %d\n"
		    "Yod partition ID: %d\n"
		    "Yod CPUs: %d",
		    n->n_yod->y_id,
		    n->n_yod->y_partid,
		    n->n_yod->y_ncpus);

		if (strcmp(n->n_yod->y_cmd, DV_NOAUTH) != 0) {
			struct buf bw_ycmd;

			text_wrap(&bw_ycmd, n->n_yod->y_cmd, 40, "\n  ", 2);
			panel_add_content(p, "\nYod command: %s", buf_get(&bw_ycmd));
			buf_free(&bw_ycmd);
		}
	}

#if 0
	panel_add_content(p,
	    "\n\n"
	    "nblk=%e %e %e %e\n"
	    "nflt=%e %e %e %e\n"
	    "npkt=%e %e %e %e",
	    n->n_sstar.ss_cnt[SSCNT_NBLK][0],
	    n->n_sstar.ss_cnt[SSCNT_NBLK][1],
	    n->n_sstar.ss_cnt[SSCNT_NBLK][2],
	    n->n_sstar.ss_cnt[SSCNT_NBLK][3],
	    n->n_sstar.ss_cnt[SSCNT_NFLT][0],
	    n->n_sstar.ss_cnt[SSCNT_NFLT][1],
	    n->n_sstar.ss_cnt[SSCNT_NFLT][2],
	    n->n_sstar.ss_cnt[SSCNT_NFLT][3],
	    n->n_sstar.ss_cnt[SSCNT_NPKT][0],
	    n->n_sstar.ss_cnt[SSCNT_NPKT][1],
	    n->n_sstar.ss_cnt[SSCNT_NPKT][2],
	    n->n_sstar.ss_cnt[SSCNT_NPKT][3]);
#endif
}

void
panel_refresh_flyby(struct panel *p)
{
	static int sav_cpanels, sav_mode = -1;
	struct panel *t;
	int cpanels;

	cpanels = 0;
	TAILQ_FOREACH(t, &panels, p_link)
		if (t->p_id & (PANEL_FBNEW | PANEL_FBCHO))
			cpanels |= t->p_id;

	if (sav_cpanels == cpanels && sav_mode == flyby_mode &&
	    panel_ready(p))
		return;
	sav_mode = flyby_mode;
	sav_cpanels = cpanels;

	pwidget_startlist(p);
	panel_set_content(p, "- Flyby -\nCurrent flyby: %s", flyby_name);
	switch (flyby_mode) {
	case FBM_PLAY:
		panel_add_content(p, "\nPlaying back flyby");
		break;
	case FBM_REC:
		panel_add_content(p, "\nRecording new flyby");

		pwidget_add(p, &fill_nodata, "Stop recording",
		    gscb_pw_fb, PWFF_STOPREC);
		break;
	default:
		pwidget_add(p, &fill_nodata, "Play",
		    gscb_pw_fb, PWFF_PLAY);
		pwidget_add(p, &fill_nodata, "Record",
		    gscb_pw_fb, PWFF_REC);
		pwidget_add(p, &fill_nodata, "Delete",
		    gscb_pw_fb, PWFF_CLR);
		pwidget_add(p,
		    (panel_for_id(PANEL_FBNEW) ?
		    &fill_white : &fill_nodata), "Create new",
		    gscb_pw_fb, PWFF_NEW);
		pwidget_add(p,
		    (panel_for_id(PANEL_FBCHO) ?
		    &fill_white : &fill_nodata),
		    (panel_for_id(PANEL_FBCHO) == NULL ?
		    "Open Chooser" : "Close Chooser"),
		    gscb_pw_fb, PWFF_OPEN);
		break;
	}
	pwidget_endlist(p);
}

void
panel_refresh_fbcho(struct panel *p)
{
	static char sav_flyby_name[NAME_MAX];
	char path[PATH_MAX];
	struct dirent *dent;
	struct fnent *fe;
	struct stat stb;
	size_t i;
	DIR *dp;

	if (panel_ready(p) && strcmp(sav_flyby_name, flyby_name) == 0)
		return;
	strncpy(sav_flyby_name, flyby_name, sizeof(sav_flyby_name) - 1);
	sav_flyby_name[sizeof(sav_flyby_name) - 1] = '\0';

	panel_set_content(p, "- Flyby Chooser -");
	pwidget_startlist(p);
	if ((dp = opendir(_PATH_FLYBYDIR)) == NULL) {
		if (errno != ENOENT)
			err(1, "%s", _PATH_FLYBYDIR);
		errno = 0;

		pwidget_add(p, &fill_white, flyby_name, NULL, 0);
	} else {
		obj_batch_start(&flyby_list);
		while ((dent = readdir(dp)) != NULL) {
			if (strcmp(dent->d_name, ".") == 0 ||
			    strcmp(dent->d_name, "..") == 0)
				continue;

			snprintf(path, sizeof(path), "%s/%s",
			    _PATH_FLYBYDIR, dent->d_name);
			if (stat(path, &stb) == -1) {
				warn("%s", path);
				continue;
			}
			if ((stb.st_mode & S_IFMT) != S_IFREG)
				continue;

			fe = obj_get(dent->d_name, &flyby_list);
			snprintf(fe->fe_name, sizeof(fe->fe_name),
			    "%s", dent->d_name);
		}
		obj_batch_end(&flyby_list);

		/* XXX: check readdir NULL/errno */

		closedir(dp);

		qsort(flyby_list.ol_fnents, flyby_list.ol_cur,
		    sizeof(struct fnent *), fe_cmp);

		for (i = 0; i < flyby_list.ol_cur; i++) {
			fe = flyby_list.ol_fnents[i];

			pwidget_add(p, (strcmp(flyby_name, fe->fe_name) ?
			    &fill_nodata : &fill_white), fe->fe_name,
			    gscb_pw_fbcho, i);
		}
	}
	pwidget_endlist(p);
}

void
panel_refresh_fbnew(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "- Flyby Creation -\n"
	    "Flyby name: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_gotonode(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "- Go to Node -\n"
	    "Node ID: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_gotojob(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "- Go to Job -\n"
	    "Job ID: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_pos(struct panel *p)
{
	struct fvec lsph, usph;

	if ((st.st_rf & RF_CAM) == 0 && panel_ready(p))
		return;

	vec_cart2sphere(&st.st_lv, &lsph);
	vec_cart2sphere(&st.st_uv, &usph);

	panel_set_content(p, "- Camera -\n"
	    "Position (%.2f,%.2f,%.2f)\n"
	    "Look (%.2f,%.2f,%.2f) (t=%.3f,p=%.3f)\n"
	    "Up (%.2f,%.2f,%.2f) (t=%.3f,p=%.3f)",
	    st.st_x, st.st_y, st.st_z,
	    st.st_lx, st.st_ly, st.st_lz,
	    lsph.fv_t, lsph.fv_p,
	    st.st_ux, st.st_uy, st.st_uz,
	    usph.fv_t, usph.fv_p);
}

void
panel_refresh_ss(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "- Record Screenshot -\n"
	    "File name: %s", buf_get(&uinp.uinp_buf));
}

#define BLINK_INTERVAL 1000000
void
panel_refresh_eggs(struct panel *p)
{
	char *s[] = {"Follow the white rabbit...\n  %s",
			"Follow the white rabbit...\n> %s"};
	static struct timeval pre = {0,0};
	static int i = 0;
	int dirty;

	dirty = panel_blink(&pre, s, 2, &i, BLINK_INTERVAL);

	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p) && !dirty)
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, s[i], buf_get(&uinp.uinp_buf));
}

void
panel_date_invalidate(__unused int a)
{
	struct panel *p;

	p = panel_for_id(PANEL_DATE);
	if (p == NULL)
		return;
//		errx(1, "internal error: date invalidate callback "
//		    "called but no date panel present");
	p->p_opts |= POPT_REFRESH;
}

#define TMBUF_SIZ 30

void
panel_refresh_date(struct panel *p)
{
	static char tmbuf[TMBUF_SIZ];
	struct tm tm;
	time_t now;

	if (panel_ready(p))
		return;

	/*
	 * If a session is live, assume directory exists.
	 * (race conditions...)
	 */
	if (ssp != NULL) {
		char fn[PATH_MAX];
		struct stat stb;

		snprintf(fn, sizeof(fn), "%s/%s",
		    _PATH_SESSIONS, ssp->ss_sid);
		if (stat(fn, &stb) == -1)
			err(1, "stat %s", fn);
		now = stb.st_mtime;
	} else if (datasrcs[DS_NODE].ds_mtime) {
		now = datasrcs[DS_NODE].ds_mtime;
	} else {
		if (time(&now) == (time_t)-1)
			err(1, "time");
	}
	localtime_r(&now, &tm);

	if (ssp == NULL)
		glutTimerFunc(1000 * (60 - tm.tm_sec),
		    panel_date_invalidate, 0);
	strftime(tmbuf, sizeof(tmbuf), date_fmt, &tm);
	panel_set_content(p, "(c) 2006 PSC\n%s", tmbuf);
}

void
panel_refresh_opts(struct panel *p)
{
	static int sav_opts;
	int i;

	if (panel_ready(p) && sav_opts == st.st_opts)
		return;
	sav_opts = st.st_opts;

	panel_set_content(p, "- Options -");

	pwidget_startlist(p);
	for (i = 0; i < NOPS; i++) {
		if (opts[i].opt_flags & OPF_HIDE)
			continue;
		pwidget_add(p, (st.st_opts & (1 << i) ?
		    &fill_white : &fill_nodata), opts[i].opt_name,
		    gscb_pw_opt, i);
	}
	pwidget_endlist(p);
}

void
panel_refresh_panels(struct panel *p)
{
	static int sav_pids;
	struct panel *pp;
	int i, pids;

	pids = 0;
	TAILQ_FOREACH(pp, &panels, p_link)
		pids |= pp->p_id;

	if (panel_ready(p) && sav_pids == pids)
		return;
	sav_pids = pids;

	panel_set_content(p, "- Panels -");

	pwidget_startlist(p);
	for (i = 0; i < NPANELS; i++) {
		if (pinfo[i].pi_opts & PF_HIDE)
			continue;
		pwidget_add(p, (pids & (1 << i) ?
		    &fill_white : &fill_nodata), pinfo[i].pi_name,
		    gscb_pw_panel, i);
	}
	pwidget_endlist(p);
}

void
panel_refresh_status(struct panel *p)
{
	const char *s;

	if (panel_ready(p))
		return;

	s = status_get();
	if (s[0] == '\0')
		s = "(empty)";
	panel_set_content(p, "- Status -\n%s", s);
}

void
panel_refresh_login(struct panel *p)
{
	static char passbuf[BUFSIZ];
	int atpass, len;
	char *s;

	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	atpass = (p->p_opts & POPT_LOGIN_ATPASS);

	if (atpass) {
		len = strlen(buf_get(&uinp.uinp_buf));
		for (s = passbuf; s - passbuf < len; s++)
			*s = '*';
		*s = '\0';
	}

	panel_set_content(p, "- Login -\n"
	    "As job details are restricted to\n"
	    "BigBen users, please login to obtain\n"
	    "access or press escape to close.\n\n"
	    "Username: %s%s%s",
	    atpass ? authbuf : buf_get(&uinp.uinp_buf),
	    atpass ? "\nPassword: " : "",
	    atpass ? passbuf : "");
}

void
panel_refresh_help(struct panel *p)
{
	static int sav_exthelp;

	if (panel_ready(p) && sav_exthelp == exthelp)
		return;
	sav_exthelp = exthelp;

	panel_set_content(p, "");
	pwidget_startlist(p);
	if (exthelp) {
		pwidget_add(p, &fill_xparent, "Panels",
		    gscb_pw_panel, baseconv(PANEL_PANELS) - 1);
		pwidget_add(p, &fill_xparent, "Options",
		    gscb_pw_panel, baseconv(PANEL_OPTS) - 1);
		pwidget_add(p, &fill_xparent, "Clear Selnodes",
		    gscb_pw_help, HF_CLRSN);
		pwidget_add(p, &fill_xparent, "Update Data",
		    gscb_pw_help, HF_UPDATE);
	} else {
		pwidget_add(p, &fill_xparent, "Help <<",
		    gscb_pw_help, HF_SHOWHELP);
	}
	pwidget_endlist(p);
}

void
panel_refresh_vmode(struct panel *p)
{
	static int sav_vmode;
	int i;

	if (panel_ready(p) && sav_vmode == st.st_vmode)
		return;
	sav_vmode = st.st_vmode;

	panel_set_content(p, "- View Mode -");
	pwidget_startlist(p);
	for (i = 0; i < NVM; i++) {
		pwidget_add(p, (st.st_vmode == i ?
		    &fill_white : &fill_nodata), vmodes[i].vm_name,
		    gscb_pw_vmode, i);
	}
	pwidget_endlist(p);
}

void
panel_refresh_dmode(struct panel *p)
{
	static int sav_dmode;
	int i;

	if (panel_ready(p) && sav_dmode == st.st_dmode)
		return;
	sav_dmode = st.st_dmode;

	panel_set_content(p, "- Data Mode -");
	pwidget_startlist(p);
	for (i = 0; i < NDM; i++) {
		if (dmodes[i].dm_name == NULL)
			continue;
		pwidget_add(p, (st.st_dmode == i ?
		    &fill_white : &fill_nodata), dmodes[i].dm_name,
		    gscb_pw_dmode, i);
	}
	pwidget_endlist(p);
}

char *ssvclabels[] = {
	"VC0",
	"VC1",
	"VC2",
	"VC3"
};

char *ssmodelabels[] = {
	"# Blocked cycles",
	"# Flits",
	"# Packets"
};

void
panel_refresh_sstar(struct panel *p)
{
	static int sav_mode, sav_vc;
	int i;

	if (panel_ready(p) && sav_mode == st.st_ssmode &&
	    sav_vc == st.st_ssvc)
		return;
	sav_mode = st.st_ssmode;
	sav_vc = st.st_ssvc;

	panel_set_content(p, "- Seastar Controls -");
	pwidget_startlist(p);
	for (i = 0; i < NVC; i++) {
		pwidget_add(p, (st.st_ssvc == i ?
		    &fill_white : &fill_nodata), ssvclabels[i],
		    gscb_pw_ssvc, i);
	}
	for (i = 0; i < NSSCNT; i++) {
		pwidget_add(p, (st.st_ssmode == i ?
		    &fill_white : &fill_nodata), ssmodelabels[i],
		    gscb_pw_ssmode, i);
	}
	pwidget_endlist(p);
}

char *pipemodelabels[] = {
	"Torus directions",
	"Routing errors"
};

void
panel_refresh_pipe(struct panel *p)
{
	static int sav_pipemode;
	int i;

	if (panel_ready(p) && sav_pipemode == st.st_pipemode)
		return;
	sav_pipemode = st.st_pipemode;

	panel_set_content(p, "- Pipe Mode -");
	pwidget_startlist(p);
	for (i = 0; i < NPM; i++) {
		pwidget_add(p, (st.st_pipemode == i ?
		    &fill_white : &fill_nodata), pipemodelabels[i],
		    gscb_pw_pipe, i);
	}
	pwidget_endlist(p);
}

void
panel_refresh_reel(struct panel *p)
{
	static char sav_reel_fn[PATH_MAX];
	struct dirent *dent;
	struct reel *rl;
	char *fn;
	size_t i;
	DIR *dp;

	if (panel_ready(p) && strcmp(reel_fn, sav_reel_fn) == 0)
		return;
	strncpy(sav_reel_fn, reel_fn, sizeof(sav_reel_fn) - 1);
	sav_reel_fn[sizeof(sav_reel_fn) - 1] = '\0';

	panel_set_content(p, "- Reel -");
	pwidget_startlist(p);
	if (flyby_mode == FBM_PLAY &&
	    st.st_opts & OP_REEL) {
		/* basename(3) is too much work. */
		if ((fn = strrchr(reel_fn, '/')) != NULL)
			fn++;
		else
			fn = reel_fn;

		panel_add_content(p, "\nFrame %d/%d\n%s %s",
		    reel_pos, reelent_list.ol_cur, fn,
		    reelent_list.ol_cur > 0 ?
		    reelent_list.ol_fnents[reel_pos]->fe_name : "N/A");
	} else if ((dp = opendir(_PATH_ARCHIVE)) == NULL) {
		panel_add_content(p, "\nNo archive reels available.");
	} else {
		obj_batch_start(&reel_list);
		while ((dent = readdir(dp)) != NULL) {
			if (strcmp(dent->d_name, ".") == 0 ||
			    strcmp(dent->d_name, "..") == 0)
				continue;

			/* XXX: check stat & ISFILE */

			rl = obj_get(dent->d_name, &reel_list);
			snprintf(rl->rl_dirname, sizeof(rl->rl_dirname),
			    "%s/%s", _PATH_ARCHIVE, dent->d_name);
			snprintf(rl->rl_name, sizeof(rl->rl_name),
			    "%s", dent->d_name);
		}
		obj_batch_end(&reel_list);
		closedir(dp);

		qsort(reel_list.ol_reels, reel_list.ol_cur,
		    sizeof(struct reel *), reel_cmp);

		for (i = 0; i < reel_list.ol_cur; i++) {
			rl = reel_list.ol_reels[i];

			pwidget_add(p, (strcmp(reel_fn, rl->rl_dirname) ?
			    &fill_nodata : &fill_white), rl->rl_name,
			    gscb_pw_reel, i);
		}

		if (i == 0)
			panel_add_content(p, "\nNone available.");
	}
	pwidget_endlist(p);
}

void
panel_refresh_wiadj(struct panel *p)
{
	static struct ivec sav_wioff, sav_winsp;

	if (panel_ready(p) &&
	    memcmp(&sav_wioff, &st.st_wioff, sizeof(sav_wioff)) == 0 &&
	    memcmp(&sav_winsp, &st.st_winsp, sizeof(sav_winsp)) == 0)
		return;
	sav_wioff = st.st_wioff;
	sav_winsp = st.st_winsp;

	panel_set_content(p,
	    "- Wired Mode Controls -\n"
	    "Space: <%d,%d,%d>\n"
	    "Offset: <%d,%d,%d>",
	    st.st_winsp.iv_x,
	    st.st_winsp.iv_y,
	    st.st_winsp.iv_z,
	    st.st_wioff.iv_x,
	    st.st_wioff.iv_y,
	    st.st_wioff.iv_z);

	pwidget_startlist(p);
	pwidget_add(p, &fill_nodata, "x Space",  gscb_pw_wiadj, SWF_NSPX);
	pwidget_add(p, &fill_nodata, "y Space",  gscb_pw_wiadj, SWF_NSPY);
	pwidget_add(p, &fill_nodata, "z Space",  gscb_pw_wiadj, SWF_NSPZ);
	pwidget_add(p, &fill_nodata, "x Offset", gscb_pw_wiadj, SWF_OFFX);
	pwidget_add(p, &fill_nodata, "y Offset", gscb_pw_wiadj, SWF_OFFY);
	pwidget_add(p, &fill_nodata, "z Offset", gscb_pw_wiadj, SWF_OFFZ);
	pwidget_endlist(p);
}

char *rtetypelabels[] = {
	"Recoverable",
	"Fatal",
	"Router"
};

void
panel_refresh_rt(struct panel *p)
{
	static int sav_rtepset, sav_rtetype;
	int i;

	if (panel_ready(p) &&
	    sav_rtepset == st.st_rtepset &&
	    sav_rtetype == st.st_rtetype)
		return;
	sav_rtepset = st.st_rtepset;
	sav_rtetype = st.st_rtetype;

	panel_set_content(p,
	    "- Routing Error Controls -\n"
	    "Port set: %s\n"
	    "Error type: %s",
	    st.st_rtepset ? "Positive" : "Negative",
	    rtetypelabels[st.st_rtetype]);

	panel_add_content(p, "\nMax errors:");
	if (memcmp(&rt_max, &rt_zero, sizeof(rt_zero)) == 0)
		panel_add_content(p, " none");
	else {
		for (i = 0; i < NRP; i++) {
			if (rt_max.rt_err[i][RT_RECOVER] ||
			    rt_max.rt_err[i][RT_FATAL] ||
			    rt_max.rt_err[i][RT_ROUTER]) {
				int needcomma = 0;

				panel_add_content(p, "\n  port %d: ", i);
				if (rt_max.rt_err[i][RT_RECOVER]) {
					panel_add_content(p, "%d recover",
					    rt_max.rt_err[i][RT_RECOVER]);
					needcomma = 1;
				}
				if (rt_max.rt_err[i][RT_FATAL]) {
					panel_add_content(p, "%s%d fatal",
					    (needcomma ? ", " : ""),
					    rt_max.rt_err[i][RT_RECOVER]);
					needcomma = 1;
				}
				if (rt_max.rt_err[i][RT_ROUTER]) {
					panel_add_content(p, "%s%d router",
					    (needcomma ? ", " : ""),
					    rt_max.rt_err[i][RT_ROUTER]);
					needcomma = 1;
				}
			}
		}
	}

	pwidget_startlist(p);

	for (i = 0; i < NRTC / 2; i++)
		pwidget_add(p, &rtpipeclass[i].nc_fill,
		    rtpipeclass[i].nc_name, NULL, 0);

	pwidget_add(p, (st.st_rtetype == RT_RECOVER ? &fill_white :
	    &fill_nodata), "Recoverable", gscb_pw_rt, SRF_RECOVER);
	pwidget_add(p, (st.st_rtetype == RT_FATAL ? &fill_white :
	    &fill_nodata), "Fatal",  gscb_pw_rt, SRF_FATAL);
	pwidget_add(p, (st.st_rtetype == RT_ROUTER ? &fill_white :
	    &fill_nodata), "Router", gscb_pw_rt, SRF_ROUTER);

	for (; i < NRTC; i++)
		pwidget_add(p, &rtpipeclass[i].nc_fill,
		    rtpipeclass[i].nc_name, NULL, 0);

	pwidget_add(p, (st.st_rtepset == RPS_NEG ?  &fill_white :
	    &fill_nodata), "Negative", gscb_pw_rt, SRF_NEG);
	pwidget_add(p, (st.st_rtepset == RPS_POS ?  &fill_white :
	    &fill_nodata), "Positive",  gscb_pw_rt, SRF_POS);
	pwidget_endlist(p);
}

void
panel_refresh_cmp(struct panel *p)
{
	if ((st.st_rf & RF_CAM) == 0 && panel_ready(p))
		return;

	panel_set_content(p,
	 "- Compass -\n"
	 "           \n"
	 "           \n"
	 "           \n"
	 "           ");
	p->p_extdrawf = panel_draw_compass;
}

void
panel_refresh_keyh(struct panel *p)
{
	static int sav_keyh;
	int j;

	if (panel_ready(p) && sav_keyh == keyh)
		return;
	sav_keyh = keyh;

	panel_set_content(p,
	    "- Key Controls -\n"
	    "Current: %s",
	    keyhtab[keyh].kh_name);

	pwidget_startlist(p);
	for (j = 0; j < NKEYH; j++)
		pwidget_add(p, (j == keyh ? &fill_white : &fill_nodata),
		    keyhtab[j].kh_name, gscb_pw_keyh, j);
	pwidget_endlist(p);
}

void
panel_refresh_dxcho(struct panel *p)
{
	static char sav_dx_name[NAME_MAX];
	char path[PATH_MAX];
	struct dirent *dent;
	struct fnent *fe;
	struct stat stb;
	size_t i;
	DIR *dp;

	if (panel_ready(p) && strcmp(sav_dx_name, dx_fn) == 0)
		return;
	strncpy(sav_dx_name, dx_fn, sizeof(sav_dx_name) - 1);
	sav_dx_name[sizeof(sav_dx_name) - 1] = '\0';

	panel_set_content(p, "- Deus Ex Chooser -");
	pwidget_startlist(p);
	if ((dp = opendir(_PATH_DXSCRIPTS)) == NULL) {
		if (errno != ENOENT)
			err(1, "%s", _PATH_DXSCRIPTS);
		errno = 0;

		pwidget_add(p, &fill_white, dx_fn, NULL, 0);
	} else {
		obj_batch_start(&dxscript_list);
		while ((dent = readdir(dp)) != NULL) {
			if (strcmp(dent->d_name, ".") == 0 ||
			    strcmp(dent->d_name, "..") == 0)
				continue;

			snprintf(path, sizeof(path), "%s/%s",
			    _PATH_DXSCRIPTS, dent->d_name);
			if (stat(path, &stb) == -1) {
				warn("%s", path);
				continue;
			}
			if ((stb.st_mode & S_IFMT) != S_IFREG)
				continue;

			fe = obj_get(dent->d_name, &dxscript_list);
			snprintf(fe->fe_name, sizeof(fe->fe_name),
			    "%s", dent->d_name);
		}
		obj_batch_end(&dxscript_list);

		/* XXX: check readdir NULL/errno */

		closedir(dp);

		qsort(dxscript_list.ol_fnents, dxscript_list.ol_cur,
		    sizeof(struct fnent *), fe_cmp);

		for (i = 0; i < dxscript_list.ol_cur; i++) {
			fe = dxscript_list.ol_fnents[i];

			pwidget_add(p, (strcmp(dx_fn, fe->fe_name) ?
			    &fill_nodata : &fill_white), fe->fe_name,
			    gscb_pw_dxcho, i);
		}
	}
	pwidget_endlist(p);
}
