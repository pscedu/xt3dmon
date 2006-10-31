/* $Id$ */

/*
 * prefresh -- guts of per-panel contents building.
 *
 * This file name should probably be named pbuild.c
 * or something instead to avoid confusion with the
 * actual drawing calculations of panels which are
 * instead done in panel.c.
 */

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
#include "capture.h"
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
#include "status.h"
#include "uinp.h"
#include "util.h"
#include "yod.h"

int	 dmode_data_clean;
int	 selnode_clean;
int	 hlnc_clean;

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
	int ready = 1;

	if (p->p_opts & POPT_REFRESH) {
		ready = 0;
		p->p_opts &= ~POPT_REFRESH;
	}
	if (p->p_info->pi_flags & PIF_UINP &&
	    uinp.uinp_opts & UINPO_DIRTY) {
		ready = 0;
		uinp.uinp_opts &= ~UINPO_DIRTY;
	}
	if (p->p_str == NULL)
		ready = 0;
	return (ready);
}

int
pwidget_cmp(const void *a, const void *b)
{
	const struct pwidget *pwa, *pwb;
	pwa = *(struct pwidget **)a;
	pwb = *(struct pwidget **)b;
	if (pwa->pw_sprio > pwb->pw_sprio)
		return (-1);
	else if (pwa->pw_sprio < pwb->pw_sprio)
		return (1);
	return (strcmp(pwa->pw_str, pwb->pw_str));
}

void
pwidget_sortlist(struct panel *p, int (*cmp)(const void *a, const void *b))
{
	struct pwidget *pw, **pws;
	int j;

	if (p->p_nwidgets == 0)
		return;

	if ((pws = malloc(p->p_nwidgets * sizeof(*pws))) == NULL)
		err(1, "malloc");
	j = 0;
	SLIST_FOREACH(pw, &p->p_widgets, pw_next)
		pws[j++] = pw;
	qsort(pws, p->p_nwidgets, sizeof(*pws), cmp);
	pw = SLIST_FIRST(&p->p_widgets) = pws[0];
	for (j = 1; j < p->p_nwidgets; j++)
		pw = SLIST_NEXT(pw, pw_next) = pws[j];
	SLIST_NEXT(pw, pw_next) = NULL;
	free(pws);
}

void
pwidget_startlist(struct panel *p)
{
	p->p_nwidgets = 0;
	SLIST_FIRST(&p->p_freewidgets) = SLIST_FIRST(&p->p_widgets);
	SLIST_INIT(&p->p_widgets);
	p->p_lastwidget = NULL;
}

void
pwidget_endlist(struct panel *p, int nwcol)
{
	struct pwidget *pw, *nextp;

	p->p_nwcol = nwcol;
	if (p->p_nwidgets == 0)
		SLIST_INIT(&p->p_widgets);
	else
		panel_calcwlens(p);

	if (SLIST_EMPTY(&p->p_freewidgets))
		return;
	for (pw = SLIST_FIRST(&p->p_freewidgets);
	    pw != SLIST_END(&p->p_freewidgets); pw = nextp) {
		nextp = SLIST_NEXT(pw, pw_next);
		free(pw);
	}
	SLIST_INIT(&p->p_freewidgets);
}

void
pwidget_add(struct panel *p, struct fill *fp, const char *s, int sprio,
    gscb_t cb, int arg_int, int arg_int2, void *arg_ptr, void *arg_ptr2)
{
	struct pwidget *pw;

	p->p_nwidgets++;
	if (SLIST_EMPTY(&p->p_freewidgets)) {
		if ((pw = malloc(sizeof(*pw))) == NULL)
			err(1, "malloc");
		memset(pw, 0, sizeof(*pw));
	} else {
		pw = SLIST_FIRST(&p->p_freewidgets);
		SLIST_REMOVE_HEAD(&p->p_freewidgets, pw_next);
		SLIST_NEXT(pw, pw_next) = NULL;
	}
	if (p->p_lastwidget)
		SLIST_NEXT(p->p_lastwidget, pw_next) = pw;
	else
		SLIST_INSERT_HEAD(&p->p_widgets, pw, pw_next);
	p->p_lastwidget = pw;

	pw->pw_fillp = fp;
	pw->pw_cb = cb;
	pw->pw_arg_int = arg_int;
	pw->pw_arg_int2 = arg_int2;
	pw->pw_arg_ptr = arg_ptr;
	pw->pw_arg_ptr2 = arg_ptr2;
	pw->pw_str = s;
	pw->pw_sprio = sprio;
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
	if (selnode_clean & PANEL_CMD && panel_ready(p))
		return;
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
	int (*cmp)(const void *, const void *);
	size_t j;
	int i;

	if (dmode_data_clean & PANEL_LEGEND &&
	    hlnc_clean & PANEL_LEGEND && panel_ready(p))
		return;
	dmode_data_clean |= PANEL_LEGEND;
	hlnc_clean |= PANEL_LEGEND;

	cmp = NULL;
	pwidget_startlist(p);
	switch (st.st_dmode) {
	case DM_JOB:
		panel_set_content(p, "- Node Legend (Jobs) -\n"
		    "Total jobs: %lu", job_list.ol_cur);

		pwidget_add(p, &fill_showall, "Show all", NSC + 1,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);

		for (j = 0; j < NSC; j++) {
			if (statusclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &statusclass[j].nc_fill,
			    statusclass[j].nc_name, NSC - j,
			    gscb_pw_hlnc, j, 0, NULL, NULL);
		}
		for (j = 0; j < job_list.ol_cur; j++)
			pwidget_add(p, &OLE(job_list, j, job)->j_fill,
			    OLE(job_list, j, job)->j_name, 0,
			    gscb_pw_hlnc, NSC + j, 0, NULL, NULL);
		cmp = pwidget_cmp;
		break;
	case DM_FAIL:
		panel_set_content(p, "- Node Legend (Failures) -\n"
		    "Total: %lu", total_failures);

		pwidget_add(p, &fill_showall, "Show all", 0,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);
		pwidget_add(p, &fill_nodata, "No data", 0,
		    NULL, 0, 0, NULL, NULL);

		for (j = 0; j < NFAILC; j++) {
			if (failclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &failclass[j].nc_fill,
			    failclass[j].nc_name, 0,
			    gscb_pw_hlnc, j, 0, NULL, NULL);
		}
		break;
	case DM_TEMP:
		panel_set_content(p, "- Node Legend (Temperature) -");

		pwidget_add(p, &fill_showall, "Show all", 0,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);
		pwidget_add(p, &fill_nodata, "No data", 0,
		    NULL, 0, 0, NULL, NULL);

		for (i = NTEMPC - 1; i >= 0; i--) {
			if (tempclass[i].nc_nmemb == 0)
				continue;
			pwidget_add(p, &tempclass[i].nc_fill,
			    tempclass[i].nc_name, 0,
			    gscb_pw_hlnc, i, 0, NULL, NULL);
		}
		break;
	case DM_YOD:
		panel_set_content(p, "- Node Legend (Yods) -\n"
		    "Total yods: %lu", yod_list.ol_cur);

		pwidget_add(p, &fill_showall, "Show all", NSC + 1,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);

		for (j = 0; j < NSC; j++) {
			if (statusclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &statusclass[j].nc_fill,
			    statusclass[j].nc_name, NSC - j,
			    gscb_pw_hlnc, j, 0, NULL, NULL);
		}
		for (j = 0; j < yod_list.ol_cur; j++)
			pwidget_add(p, &OLE(yod_list, j, yod)->y_fill,
			    OLE(yod_list, j, yod)->y_cmd, 0,
			    gscb_pw_hlnc, NSC + j, 0, NULL, NULL);
		cmp = pwidget_cmp;
		break;
	case DM_RTUNK:
		panel_set_content(p, "- Node Legend (Route Errors) -");

		pwidget_add(p, &fill_showall, "Show all", 0,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);
		pwidget_add(p, &fill_rtesnd, "Sender", 0,
		    gscb_pw_hlnc, RTC_SND, 0, NULL, NULL);
		pwidget_add(p, &fill_rtercv, "Target", 0,
		    gscb_pw_hlnc, RTC_RCV, 0, NULL, NULL);

		for (j = 0; j < NRTC; j++) {
			if (rtclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &rtclass[j].nc_fill,
			    rtclass[j].nc_name, 0,
			    gscb_pw_hlnc, j, 0, NULL, NULL);
		}
		break;
	case DM_SEASTAR:
		panel_set_content(p, "- Node Legend (SeaStar) -");

		pwidget_add(p, &fill_showall, "Show all", 0,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);

		for (j = 0; j < NSSC; j++) {
			if (ssclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &ssclass[j].nc_fill,
			    ssclass[j].nc_name, 0,
			    gscb_pw_hlnc, j, 0, NULL, NULL);
		}
		break;
	case DM_SAME:
		panel_set_content(p, "- Node Legend -");

		pwidget_add(p, &fill_same, "All nodes", 0,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);
		break;
	case DM_LUSTRE:
		panel_set_content(p, "- Node Legend (Lustre) -");

		pwidget_add(p, &fill_showall, "Show all", 0,
		    gscb_pw_hlnc, NC_ALL, 0, NULL, NULL);

		for (j = 0; j < NLUSTC; j++) {
			if (lustreclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &lustreclass[j].nc_fill,
			    lustreclass[j].nc_name, 0,
			    gscb_pw_hlnc, j, 0, NULL, NULL);
		}
		break;
	default:
		panel_set_content(p, "- Node Legend -\n"
		    "Not available.");
		break;
	}
	if (cmp)
		pwidget_sortlist(p, cmp);
	pwidget_endlist(p, 2);
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

	pwidget_startlist(p);
	if (SLIST_EMPTY(&selnodes)) {
		panel_set_content(p, "- Node Information -\n"
		    "No node(s) selected");
		goto done;
	}

	if (nselnodes > 1) {
		struct buf b_nids, bw_nids, b_data, bw_data;
		size_t i;

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

		for (i = 0; ol && i < ol->ol_cur; i++) {
			ohp = ol->ol_data[i];
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
		goto done;
	}

	/* Only one selected node. */
	n = SLIST_FIRST(&selnodes)->sn_nodep;
	node_physpos(n, &pc);
	iv = &n->n_wiv;

	panel_set_content(p,
	    "- Node Information -\n"
	    "Node ID: %d\n"
	    "Hardware name: c%d-%dc%ds%dn%d\n"
	    "Wired position: <%d,%d,%d>\n"
	    "Status: %s",
	    n->n_nid,
	    pc.pc_cb, pc.pc_r, pc.pc_cg, pc.pc_m, pc.pc_n,
	    iv->iv_x, iv->iv_y, iv->iv_z,
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
			int shown;

			panel_add_content(p, "\n port recover fatal router");
			for (j = 0; j < NRP; j++) {
				if (!n->n_route.rt_err[j][RT_RECOVER] &&
				    !n->n_route.rt_err[j][RT_FATAL] &&
				    !n->n_route.rt_err[j][RT_ROUTER])
					continue;

				if (j == RP_NEGX ||
				    j == RP_NEGY ||
				    j == RP_NEGZ)
					shown = st.st_rtepset == RPS_NEG &&
					    st.st_pipemode == PM_RTE &&
					    (st.st_opts & OP_PIPES);
				else if (j != RP_UNK)
					shown = st.st_rtepset == RPS_POS &&
					    st.st_pipemode == PM_RTE &&
					    (st.st_opts & OP_PIPES);
				else
					shown = (st.st_dmode == DM_RTUNK);

				panel_add_content(p, "\n  %s%d: %7d %5d %6d",
				    shown ? "*" : " ", j,
				    n->n_route.rt_err[j][RT_RECOVER],
				    n->n_route.rt_err[j][RT_FATAL],
				    n->n_route.rt_err[j][RT_ROUTER]);
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

	pwidget_startlist(p);
	if (n->n_temp != DV_NODATA) {
		j = roundclass(n->n_temp, TEMP_MIN, TEMP_MAX, NTEMPC);
		pwidget_add(p, &tempclass[j].nc_fill, "Show temp range", 0,
		    gscb_pw_dmnc, DM_TEMP, j, NULL, NULL);
	}
	if (n->n_yod) {
		yod_findbyid(n->n_yod->y_id, &j);
		pwidget_add(p, &n->n_yod->y_fill, "Show yod", 0,
		    gscb_pw_dmnc, DM_YOD, NSC + j, NULL, NULL);
	}
	if (n->n_job) {
		job_findbyid(n->n_job->j_id, &j);
		pwidget_add(p, &n->n_job->j_fill, "Show job", 0,
		    gscb_pw_dmnc, DM_JOB, NSC + j, NULL, NULL);
	} else
		pwidget_add(p, &statusclass[n->n_state].nc_fill, "Show class", 0,
		    gscb_pw_dmnc, DM_JOB, n->n_state, NULL, NULL);
done:
	pwidget_endlist(p, 2);
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
	panel_set_content(p, "- Flyby Controls -\nCurrent flyby: %s",
	    smart_basename(flyby_fn));
	switch (flyby_mode) {
	case FBM_PLAY:
		panel_add_content(p, "\nPlaying back flyby");
		break;
	case FBM_REC:
		panel_add_content(p, "\nRecording new flyby");

		pwidget_add(p, &fill_nodata, "Stop recording", 0,
		    gscb_pw_fb, PWFF_STOPREC, 0, NULL, NULL);
		break;
	default:
		pwidget_add(p, &fill_nodata, "Play", 0,
		    gscb_pw_fb, PWFF_PLAY, 0, NULL, NULL);
		pwidget_add(p, &fill_nodata, "Record", 0,
		    gscb_pw_fb, PWFF_REC, 0, NULL, NULL);
		pwidget_add(p, &fill_nodata, "Delete", 0,
		    gscb_pw_fb, PWFF_CLR, 0, NULL, NULL);
		pwidget_add(p, (panel_for_id(PANEL_FBNEW) ?
		    &fill_white : &fill_nodata), "Create new", 0,
		    gscb_pw_fb, PWFF_NEW, 0, NULL, NULL);
		pwidget_add(p, (panel_for_id(PANEL_FBCHO) ?
		    &fill_white : &fill_nodata), "Chooser", 0,
		    gscb_pw_fb, PWFF_OPEN, 0, NULL, NULL);
		break;
	}
	pwidget_endlist(p, 2);
}

void
panel_refresh_fbnew(struct panel *p)
{
	if (panel_ready(p))
		return;

	panel_set_content(p, "- Flyby Creator -\n"
	    "Flyby name: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_gotonode(struct panel *p)
{
	if (panel_ready(p))
		return;

	panel_set_content(p, "- Go to Node -\n"
	    "Node ID: %s", buf_get(&uinp.uinp_buf));
}

void
panel_refresh_gotojob(struct panel *p)
{
	if (panel_ready(p))
		return;

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
	if (panel_ready(p))
		return;

	panel_set_content(p, "- Record Screenshot -\n"
	    "File name: %s", buf_get(&uinp.uinp_buf));
	pwidget_startlist(p);
	pwidget_add(p, capture_usevirtual ?
	    &fill_unchecked : &fill_checked, "Window Size", 0,
	    gscb_pw_snap, 0, 0, NULL, NULL);

#define pwssres(w, h)						\
	pwidget_add(p, capture_usevirtual &&			\
	    virtwinv.iv_w == (w) && virtwinv.iv_h == (h) ?	\
	    &fill_checked : &fill_unchecked, #w "x" #h, 0,	\
	    gscb_pw_snap, (w), (h), NULL, NULL);
	pwssres(7200, 5400);
	pwssres(5600, 4200);
	pwssres(3600, 2700);
	pwssres(3600, 2400);
	pwssres(2560, 1600);
	pwssres(2048, 1536);
	pwssres(1920, 1440);
	pwssres(1600, 1200);
#undef pwssres

	pwidget_endlist(p, 2);
}

void
panel_refresh_eggs(struct panel *p)
{
	if (panel_ready(p))
		return;

	panel_set_content(p, "Follow the white rabbit...\n\n> %s",
	    buf_get(&uinp.uinp_buf));
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

		snprintf(fn, sizeof(fn), "%s/%s/%s",
		    _PATH_SESSIONS, ssp->ss_sid,
		    datasrcs[DS_NODE].ds_name);
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
		if (options[i].opt_flags & OPF_HIDE)
			continue;
		pwidget_add(p, (st.st_opts & (1 << i) ?
		    &fill_white : &fill_nodata), options[i].opt_name, 0,
		    gscb_pw_opt, i, 0, NULL, NULL);
	}
	pwidget_sortlist(p, pwidget_cmp);
	pwidget_endlist(p, 2);
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
		if (pinfo[i].pi_flags & PIF_HIDE)
			continue;
		pwidget_add(p, (pids & (1 << i) ?
		    &fill_white : &fill_nodata), pinfo[i].pi_name, 0,
		    gscb_pw_panel, i, 0, NULL, NULL);
	}
	pwidget_sortlist(p, pwidget_cmp);
	pwidget_endlist(p, 2);
}

void
panel_refresh_status(struct panel *p)
{
	struct status_log *sl;

	if (panel_ready(p))
		return;

	panel_set_content(p, "- Status Log -");
	TAILQ_FOREACH(sl, &status_hd, sl_link)
		panel_add_content(p, "\n%s", sl->sl_str);
	if (TAILQ_EMPTY(&status_hd))
		panel_add_content(p, "\n(empty)");
}

void
panel_refresh_login(struct panel *p)
{
	static char passbuf[BUFSIZ];
	int atpass, len;
	char *s;

	if (panel_ready(p))
		return;

	atpass = (p->p_opts & POPT_LOGIN_ATPASS);

	if (atpass) {
		len = strlen(buf_get(&uinp.uinp_buf));
		for (s = passbuf; s - passbuf < len; s++)
			*s = '*';
		*s = '\0';
	}

	panel_set_content(p, "- Login -\n"
	    "Username: %s%s%s",
	    atpass ? authbuf : buf_get(&uinp.uinp_buf),
	    atpass ? "\nPassword: " : "",
	    atpass ? passbuf : "");
}

void
panel_refresh_help(struct panel *p)
{
	static int sav_exthelp;

	if (sav_exthelp == exthelp &&
	    selnode_clean & PANEL_HELP &&
	    panel_ready(p))
		return;
	sav_exthelp = exthelp;
	selnode_clean |= PANEL_HELP;

	panel_set_content(p, "");
	pwidget_startlist(p);
	if (exthelp) {
		pwidget_add(p, &fill_xparent, "Panels", 0,
		    gscb_pw_panel, baseconv(PANEL_PANELS) - 1, 0, NULL, NULL);
		pwidget_add(p, &fill_xparent, "Options", 0,
		    gscb_pw_panel, baseconv(PANEL_OPTS) - 1, 0, NULL, NULL);
		pwidget_add(p, &fill_xparent, "Reorient", 0,
		    gscb_pw_help, HF_REORIENT, 0, NULL, NULL);
		pwidget_add(p, &fill_xparent, "Update Data", 0,
		    gscb_pw_help, HF_UPDATE, 0, NULL, NULL);
		if (nselnodes) {
			pwidget_add(p, &fill_xparent, "Clear Selnodes", 0,
			    gscb_pw_help, HF_CLRSN, 0, NULL, NULL);
			pwidget_add(p, &fill_xparent, "Print Selnode IDs", 0,
			    gscb_pw_help, HF_PRTSN, 0, NULL, NULL);
			pwidget_add(p, &fill_xparent, "Subselect Selnodes", 0,
			    gscb_pw_help, HF_SUBSN, 0, NULL, NULL);
		}
	} else {
		pwidget_add(p, &fill_xparent, "Help <<", 0,
		    gscb_pw_help, HF_SHOWHELP, 0, NULL, NULL);
	}
	pwidget_endlist(p, 2);
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
		    &fill_white : &fill_nodata), vmodes[i].vm_name, 0,
		    gscb_pw_vmode, i, 0, NULL, NULL);
	}
	pwidget_endlist(p, 1);
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
		    &fill_white : &fill_nodata), dmodes[i].dm_name, 0,
		    gscb_pw_dmode, i, 0, NULL, NULL);
	}
	pwidget_endlist(p, 2);
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
		    &fill_white : &fill_nodata), ssvclabels[i], 0,
		    gscb_pw_ssvc, i, 0, NULL, NULL);
	}
	for (i = 0; i < NSSCNT; i++) {
		pwidget_add(p, (st.st_ssmode == i ?
		    &fill_white : &fill_nodata), ssmodelabels[i], 0,
		    gscb_pw_ssmode, i, 0, NULL, NULL);
	}
	pwidget_endlist(p, 2);
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
		    &fill_white : &fill_nodata), pipemodelabels[i], 0,
		    gscb_pw_pipe, i, 0, NULL, NULL);
	}
	pwidget_endlist(p, 1);
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
	pwidget_add(p, &fill_nodata, "x Space", 0,
	    gscb_pw_wiadj, SWF_NSPX, 0, NULL, NULL);
	pwidget_add(p, &fill_nodata, "y Space", 0,
	    gscb_pw_wiadj, SWF_NSPY, 0, NULL, NULL);
	pwidget_add(p, &fill_nodata, "z Space", 0,
	    gscb_pw_wiadj, SWF_NSPZ, 0, NULL, NULL);
	pwidget_add(p, &fill_nodata, "x Offset", 0,
	    gscb_pw_wiadj, SWF_OFFX, 0, NULL, NULL);
	pwidget_add(p, &fill_nodata, "y Offset", 0,
	    gscb_pw_wiadj, SWF_OFFY, 0, NULL, NULL);
	pwidget_add(p, &fill_nodata, "z Offset", 0,
	    gscb_pw_wiadj, SWF_OFFZ, 0, NULL, NULL);
	pwidget_endlist(p, 2);
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

	panel_set_content(p, "- Routing Error Controls -");

	if (memcmp(&rt_max, &rt_zero, sizeof(rt_zero)) == 0)
		panel_add_content(p, "\nNo error statistics.");
	else {
		int total[NRT];

		memset(total, 0, sizeof(total));
		panel_add_content(p, "\nTotal:\n port recover fatal router");
		for (i = 0; i < NRP; i++) {
			if (rt_max.rt_err[i][RT_RECOVER] ||
			    rt_max.rt_err[i][RT_FATAL] ||
			    rt_max.rt_err[i][RT_ROUTER]) {
				panel_add_content(p,
				    "\n %3d: %7d %5d %6d", i,
				    rt_max.rt_err[i][RT_RECOVER],
				    rt_max.rt_err[i][RT_FATAL],
				    rt_max.rt_err[i][RT_ROUTER]);
				total[RT_RECOVER] += rt_max.rt_err[i][RT_RECOVER];
				total[RT_FATAL]   += rt_max.rt_err[i][RT_FATAL];
				total[RT_ROUTER]  += rt_max.rt_err[i][RT_ROUTER];
			}
		}
		panel_add_content(p,
		    "\n all: %7d %5d %6d",
		    total[RT_RECOVER],
		    total[RT_FATAL],
		    total[RT_ROUTER]);
	}

	pwidget_startlist(p);

	pwidget_add(p, (st.st_rtetype == RT_RECOVER ?
	    &fill_white : &fill_nodata), "Recover", 0,
	    gscb_pw_rt, SRF_RECOVER, 0, NULL, NULL);
	pwidget_add(p, (st.st_rtetype == RT_FATAL ?
	    &fill_white : &fill_nodata), "Fatal", 0,
	    gscb_pw_rt, SRF_FATAL, 0, NULL, NULL);
	pwidget_add(p, (st.st_rtetype == RT_ROUTER ?
	    &fill_white : &fill_nodata), "Router", 0,
	    gscb_pw_rt, SRF_ROUTER, 0, NULL, NULL);
	pwidget_add(p, &fill_nopanel, "", 0, NULL, 0, 0, NULL, NULL);
	pwidget_add(p, &fill_nopanel, "----- Pipe", 0, NULL, 0, 0, NULL, NULL);

	for (i = 0; i < NRTC / 2; i++)
		pwidget_add(p, &rtpipeclass[i].nc_fill,
		    rtpipeclass[i].nc_name, 0, NULL, 0, 0, NULL, NULL);

	pwidget_add(p, (st.st_rtepset == RPS_NEG ?
	    &fill_white : &fill_nodata), "Negative", 0,
	    gscb_pw_rt, SRF_NEG, 0, NULL, NULL);
	pwidget_add(p, (st.st_rtepset == RPS_POS ?
	    &fill_white : &fill_nodata), "Positive", 0,
	    gscb_pw_rt, SRF_POS, 0, NULL, NULL);
	pwidget_add(p, &fill_nopanel, "", 0, NULL, 0, 0, NULL, NULL);
	pwidget_add(p, &fill_nopanel, "", 0, NULL, 0, 0, NULL, NULL);
	pwidget_add(p, &fill_nopanel, "Legend ---", 0, NULL, 0, 0, NULL, NULL);

	for (; i < NRTC; i++)
		pwidget_add(p, &rtpipeclass[i].nc_fill,
		    rtpipeclass[i].nc_name, 0, NULL, 0, 0, NULL, NULL);

	pwidget_endlist(p, 2);
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

	panel_set_content(p, "- Key Controls -");
	pwidget_startlist(p);
	for (j = 0; j < NKEYH; j++)
		pwidget_add(p, (j == keyh ? &fill_white : &fill_nodata),
		    keyhtab[j].kh_name, 0, gscb_pw_keyh, j, 0, NULL, NULL);
	pwidget_endlist(p, 1);
}

#define PWDF_DIRSONLY (1<<0)

/*
 * Add pwidgets for all files in the given directory.
 * 'cur' is the currently selected file name.
 * 'dir' must point to a PATH_MAX-sized buffer.
 */
void
pwidgets_dir(struct panel *p, const char *dir, struct objlist *ol,
    char *cur, void *cb, int flags)
{
	char path[PATH_MAX];
	struct dirent *dent;
	struct fnent *fe;
	struct stat stb;
	int isdir;
	DIR *dp;

	if ((dp = opendir(dir)) == NULL) {
		if (errno != ENOENT)
			err(1, "%s", dir);
		errno = 0;
		return;
	}
	obj_batch_start(ol);
	while ((dent = readdir(dp)) != NULL) {
		if (dent->d_name[0] == '.') {
#if 0
			/*
			 * XXX: must clear out
			 * save_*_dir to reload dir.
			 */
			if (strcmp(dent->d_name, ".") == 0) {
				pe = obj_get(dent->d_name, ol);
				snprintf(fe->fe_name,
				    sizeof(fe->fe_name),
				    "%s", dir);
				pwidget_add(p, &fill_black,
				    "Current Directory", 2,
				    gscb_pw_dir, 1, 0,
				    cb, fe->fe_name);
			} else
#endif
			if (strcmp(dent->d_name, "..") == 0) {
				fe = obj_get(dent->d_name, ol);
				snprintf(fe->fe_name,
				    sizeof(fe->fe_name), "..");
				pwidget_add(p, &fill_xparent, "Up", 2,
				    gscb_pw_dir, 1, 0, cb, fe->fe_name);
			}
			continue;
		}
		if (strcmp(dent->d_name, "CVS") == 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s",
		    dir, dent->d_name);
		if (stat(path, &stb) == -1) {
			warn("stat %s", path);
			continue;
		}
		isdir = S_ISDIR(stb.st_mode);
		if (!isdir && !S_ISREG(stb.st_mode))
			continue;
		if (!isdir && (flags & PWDF_DIRSONLY))
			continue;

		fe = obj_get(dent->d_name, ol);
		snprintf(fe->fe_name, sizeof(fe->fe_name), "%s%s",
		    dent->d_name, isdir ? "/" : "");
		pwidget_add(p, (strcmp(cur, path) ?
		    (isdir ? &fill_xparent : &fill_nodata) : &fill_white),
		    fe->fe_name, isdir, gscb_pw_dir, isdir, 0, cb,
		    fe->fe_name);
	}
	obj_batch_end(ol);
//	if (ret == -1)
//		err(1, "readdir %s", dir);
	closedir(dp);
}

void
panel_refresh_fbcho(struct panel *p)
{
	if (panel_ready(p))
		return;

	panel_set_content(p, "- Flyby Chooser -\n%s", flyby_dir);
	pwidget_startlist(p);
	pwidgets_dir(p, flyby_dir, &flyby_list, flyby_fn, flyby_set, 0);
#if 0
	if (panel->p_nwidgets == 0)
		pwidget_add(p, &fill_white, FLYBY_DEFAULT, 0, );
#endif
	pwidget_sortlist(p, pwidget_cmp);
	pwidget_endlist(p, 2);
}

void
panel_refresh_dxcho(struct panel *p)
{
	if (panel_ready(p))
		return;

	panel_set_content(p, "- Deus Ex Chooser -\n%s", dx_dir);
	pwidget_startlist(p);
	pwidgets_dir(p, dx_dir, &dxscript_list, dx_fn, dx_set, 0);
	if (p->p_nwidgets == 0)
		panel_add_content(p, "\nNo scripts available.");
	pwidget_sortlist(p, pwidget_cmp);
	pwidget_endlist(p, 2);
}

void
panel_refresh_reel(struct panel *p)
{
	static size_t save_reel_pos;

	if (panel_ready(p) && save_reel_pos == reel_pos)
		return;
	save_reel_pos = reel_pos;

	panel_set_content(p, "- Reel -");
	pwidget_startlist(p);
	if ((flyby_mode == FBM_PLAY || dx_active) &&
	    st.st_opts & OP_REEL) {
		panel_add_content(p, "\n%s\nFrame %d/%d\n%s",
		    smart_basename(reel_dir), reel_pos + 1,
		    reelframe_list.ol_cur,
		    reelframe_list.ol_cur > 0 ?
		    OLE(reelframe_list, reel_pos, fnent)->fe_name : "N/A");
	} else {
		if (strcmp(reel_dir, "") == 0)
			panel_add_content(p, "\nNo reel selected");
		else
			panel_add_content(p, "\n%s: %d frames",
			    smart_basename(reel_dir),
			    reelframe_list.ol_cur);
		panel_add_content(p, "\n\n%s", reel_browsedir);
		pwidgets_dir(p, reel_browsedir, &reel_list, reel_dir,
		    reel_set, PWDF_DIRSONLY);
//		if (p->p_nwidgets == 0)
//			panel_add_content(p, "\nNo archive reels available.");
	}
	pwidget_sortlist(p, pwidget_cmp);
	pwidget_endlist(p, 2);
}

void
panel_refresh_dscho(struct panel *p)
{
	int live, ncols;

	if (panel_ready(p))
		return;

	live = dsfopts & DSFO_LIVE;
	panel_set_content(p, "- Dataset Chooser -\n%s\n\n%s:",
	     live ? "live" : ds_dir, ds_browsedir);
	pwidget_startlist(p);
	pwidget_add(p, (live ? &fill_white : &fill_nodata),
	    "Live", 2, gscb_pw_dscho, 0, 0, NULL, NULL);
	pwidgets_dir(p, ds_browsedir, &ds_list, live ? "" : ds_dir,
	    ds_set, PWDF_DIRSONLY);
	pwidget_sortlist(p, pwidget_cmp);
	if (ds_list.ol_tcur < 35)
		ncols = 2;
	else if (ds_list.ol_tcur < 100)
		ncols = 3;
	else if (ds_list.ol_tcur < 160)
		ncols = 4;
	else
		ncols = 5;
	pwidget_endlist(p, ncols);
}
