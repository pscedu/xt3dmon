/* $Id$ */

/*
 * prefresh -- guts of per-panel contents building.
 *
 * This file name should probably be named pbuild.c
 * or something instead to avoid confusion with the
 * actual drawing calculations of panels which are
 * instead done in panel.c.
 *
 * Panel Widget Groups
 *	Pwidgets can optionallly be sorted into groups
 * 	where the only added functionality is the use of
 *	the arrow keys to flip between choices.  In order
 * 	to specify this functionality in the refresh
 * 	routines, the following API is provided:
 *
 *	  pwidget_group_start()
 *		Start a new group.  All widgets added
 *		between this and its corresponding end()
 *		call will be in the group.
 *
 *	  pwidget_group_end()
 *		End the current widget group.
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
#include "mach.h"
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

void
pwidget_group_start(struct panel *p)
{
	struct pwidget_group *pwg;

	if ((pwg = malloc(sizeof(*pwg))) == NULL)
		err(1, "malloc");
	memset(pwg, 0, sizeof(*pwg));
	TAILQ_INIT(&pwg->pwg_widgets);
	SLIST_INSERT_HEAD(&p->p_widgetgroups, pwg, pwg_link);
	p->p_curwidgetgroup = pwg;
}

void
pwidget_group_end(struct panel *p)
{
	p->p_curwidgetgroup = NULL;
}

void
pwidget_grouplist_free(struct panel *p)
{
	struct pwidget_group *pwg, *npwg;

	for (pwg = SLIST_FIRST(&p->p_widgetgroups);
	    pwg != SLIST_END(&p->p_widget); pwg = npwg) {
		npwg = SLIST_NEXT(pwg, pwg_link);
		free(pwg);
	}
	SLIST_INIT(&p->p_widgetgroups);
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

	pwidget_grouplist_free(p);
}

void
pwidget_endlist(struct panel *p, int nwcol)
{
	struct pwidget *pw, *nextp;

	/* Build panel widget group links. */
	SLIST_FOREACH(pw, &p->p_widgets, pw_next)
		if (pw->pw_group)
			TAILQ_INSERT_TAIL(&pw->pw_group->pwg_widgets,
			    pw, pw_group_link);

	/* Calculate panel widget column widths. */
	p->p_nwcol = nwcol;
	if (p->p_nwidgets == 0)
		SLIST_INIT(&p->p_widgets);
	else
		panel_calcwlens(p);


	/* Free previously allocated widgets. */
	if (!SLIST_EMPTY(&p->p_freewidgets)) {
		for (pw = SLIST_FIRST(&p->p_freewidgets);
		    pw != SLIST_END(&p->p_freewidgets); pw = nextp) {
			nextp = SLIST_NEXT(pw, pw_next);
			free(pw);
		}
		SLIST_INIT(&p->p_freewidgets);
	}
}

#define PWARG_GSCB		0
#define PWARG_SPRIO		1
#define PWARG_CBARG_INT		2
#define PWARG_CBARG_INT2	3
#define PWARG_CBARG_PTR		4
#define PWARG_CBARG_PTR2	5
#define PWARG_GRP_CHECKED	6
#define PWARG_GRP_MEMBER	7
#define PWARG_LAST		8

void
pwidget_add(struct panel *p, struct fill *fp, const char *s, ...)
{
	int argt, grp_member, grp_checked;
	struct pwidget *pw;
	va_list ap;

	p->p_nwidgets++;
	if (SLIST_EMPTY(&p->p_freewidgets)) {
		if ((pw = malloc(sizeof(*pw))) == NULL)
			err(1, "malloc");
	} else {
		pw = SLIST_FIRST(&p->p_freewidgets);
		SLIST_REMOVE_HEAD(&p->p_freewidgets, pw_next);
	}
	memset(pw, 0, sizeof(*pw));
	if (p->p_lastwidget)
		SLIST_INSERT_AFTER(p->p_lastwidget, pw, pw_next);
	else
		SLIST_INSERT_HEAD(&p->p_widgets, pw, pw_next);
	p->p_lastwidget = pw;

	pw->pw_fillp = fp;
	pw->pw_str = s;

	grp_member = 1;
	grp_checked = 0;
	va_start(ap, s);
	do {
		argt = va_arg(ap, int);
		switch (argt) {
		case PWARG_GSCB:
			pw->pw_cb = va_arg(ap, void *);
			break;
		case PWARG_SPRIO:
			pw->pw_sprio = va_arg(ap, int);
			break;
		case PWARG_CBARG_INT:
			pw->pw_arg_int = va_arg(ap, int);
			break;
		case PWARG_CBARG_INT2:
			pw->pw_arg_int2 = va_arg(ap, int);
			break;
		case PWARG_CBARG_PTR:
			pw->pw_arg_ptr = va_arg(ap, void *);
			break;
		case PWARG_CBARG_PTR2:
			pw->pw_arg_ptr2 = va_arg(ap, void *);
			break;
		case PWARG_GRP_CHECKED:
			grp_checked = va_arg(ap, int);
			break;
		case PWARG_GRP_MEMBER:
			grp_member = va_arg(ap, int);
			break;
		}
	} while (argt != PWARG_LAST);
	va_end(ap);

	if (grp_member) {
		pw->pw_group = p->p_curwidgetgroup;
		if (grp_checked)
			p->p_curwidgetgroup->pwg_checkedwidget = pw;
	}
}

void
panel_refresh_fps(struct panel *p)
{
	if (panel_ready(p))
		return;
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
	int i, sawchecked, ischecked;
	size_t j;

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

		pwidget_add(p, &fill_showall, "Show all",
		    PWARG_SPRIO, NSC + 1,
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);

		sawchecked = 0;
		pwidget_group_start(p);
		for (j = 0; j < NSC; j++) {
			if (statusclass[j].nc_nmemb == 0)
				continue;
			ischecked = (!sawchecked && nc_getfp(j)->f_a ? 1 : 0);
			if (ischecked)
				sawchecked = 1;
			pwidget_add(p, &statusclass[j].nc_fill,
			    statusclass[j].nc_name,
			    PWARG_SPRIO, NSC - j,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_GRP_CHECKED, ischecked,
			    PWARG_CBARG_INT, j, PWARG_LAST);
		}
		for (j = 0; j < job_list.ol_cur; j++) {
			ischecked = (!sawchecked && nc_getfp(NSC + j)->f_a ? 1 : 0);
			if (ischecked)
				sawchecked = 1;
			pwidget_add(p, &OLE(job_list, j, job)->j_fill,
			    OLE(job_list, j, job)->j_name,
			    PWARG_SPRIO, 0,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_GRP_CHECKED, ischecked,
			    PWARG_CBARG_INT, NSC + j, PWARG_LAST);
		}
		pwidget_group_end(p);

		cmp = pwidget_cmp;
		break;
	case DM_TEMP:
		panel_set_content(p, "- Node Legend (Temperature) -");

		pwidget_add(p, &fill_showall, "Show all",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);
		pwidget_add(p, &fill_nodata, "No data", PWARG_LAST);

		for (i = NTEMPC - 1; i >= 0; i--) {
			if (tempclass[i].nc_nmemb == 0)
				continue;
			pwidget_add(p, &tempclass[i].nc_fill,
			    tempclass[i].nc_name,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_CBARG_INT, i, PWARG_LAST);
		}
		break;
	case DM_YOD:
		panel_set_content(p, "- Node Legend (Yods) -\n"
		    "Total yods: %lu", yod_list.ol_cur);

		pwidget_add(p, &fill_showall, "Show all",
		    PWARG_SPRIO, NSC + 1,
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);

		pwidget_group_start(p);
		for (j = 0; j < NSC; j++) {
			if (statusclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &statusclass[j].nc_fill,
			    statusclass[j].nc_name,
			    PWARG_SPRIO, NSC - j,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_CBARG_INT, j, PWARG_LAST);
		}
		for (j = 0; j < yod_list.ol_cur; j++)
			pwidget_add(p, &OLE(yod_list, j, yod)->y_fill,
			    OLE(yod_list, j, yod)->y_cmd,
			    PWARG_SPRIO, 0,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_CBARG_INT, NSC + j, PWARG_LAST);
		pwidget_group_end(p);

		cmp = pwidget_cmp;
		break;
	case DM_RTUNK:
		panel_set_content(p, "- Node Legend (Route Errors) -");

		pwidget_add(p, &fill_showall, "Show all",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);
		pwidget_add(p, &fill_rtesnd, "Sender",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, RTC_SND, PWARG_LAST);
		pwidget_add(p, &fill_rtercv, "Target",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, RTC_RCV, PWARG_LAST);

		for (j = 0; j < NRTC; j++) {
			if (rtclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &rtclass[j].nc_fill,
			    rtclass[j].nc_name,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_CBARG_INT, j, PWARG_LAST);
		}
		break;
	case DM_SEASTAR:
		panel_set_content(p, "- Node Legend (SeaStar) -");

		pwidget_add(p, &fill_showall, "Show all",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);

		for (j = 0; j < NSSC; j++) {
			if (ssclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &ssclass[j].nc_fill,
			    ssclass[j].nc_name,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_CBARG_INT, j, PWARG_LAST);
		}
		break;
	case DM_SAME:
		panel_set_content(p, "- Node Legend -");

		pwidget_add(p, &fill_same, "All nodes",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);
		break;
	case DM_LUSTRE:
		panel_set_content(p, "- Node Legend (Lustre) -");

		pwidget_add(p, &fill_showall, "Show all",
		    PWARG_GSCB, gscb_pw_hlnc,
		    PWARG_CBARG_INT, NC_ALL, PWARG_LAST);

		for (j = 0; j < NLUSTC; j++) {
			if (lustreclass[j].nc_nmemb == 0)
				continue;
			pwidget_add(p, &lustreclass[j].nc_fill,
			    lustreclass[j].nc_name,
			    PWARG_GSCB, gscb_pw_hlnc,
			    PWARG_CBARG_INT, j, PWARG_LAST);
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

	if (n->n_job || n->n_yod)
		panel_add_content(p, "\nCores in use: %d/%d",
		    n->n_yod && n->n_yod->y_single ? 1 : 2,
		    machine.m_coredim.iv_x *
		    machine.m_coredim.iv_y *
		    machine.m_coredim.iv_z);

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
		if (n->n_job->j_mem) {
			char membuf[FMT_SCALED_BUFSIZ];

			fmt_scaled(n->n_job->j_mem * 1024, membuf);
			panel_add_content(p,
			    "\nJob Login Node Memory: %s", membuf);
		}
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
		pwidget_add(p, &tempclass[j].nc_fill, "Show temp range",
		    PWARG_GSCB, gscb_pw_dmnc,
		    PWARG_CBARG_INT, DM_TEMP,
		    PWARG_CBARG_INT2, j, PWARG_LAST);
	}
	if (n->n_yod) {
		yod_findbyid(n->n_yod->y_id, &j);
		pwidget_add(p, &n->n_yod->y_fill, "Show yod",
		    PWARG_GSCB, gscb_pw_dmnc,
		    PWARG_CBARG_INT, DM_YOD,
		    PWARG_CBARG_INT2, NSC + j, PWARG_LAST);
	}
	if (n->n_job) {
		job_findbyid(n->n_job->j_id, &j);
		pwidget_add(p, &n->n_job->j_fill, "Show job",
		    PWARG_GSCB, gscb_pw_dmnc,
		    PWARG_CBARG_INT, DM_JOB,
		    PWARG_CBARG_INT2, NSC + j, PWARG_LAST);
	} else
		pwidget_add(p, &statusclass[n->n_state].nc_fill, "Show class",
		    PWARG_GSCB, gscb_pw_dmnc,
		    PWARG_CBARG_INT, DM_JOB,
		    PWARG_CBARG_INT2, n->n_state, PWARG_LAST);
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

		pwidget_add(p, &fill_button, "Stop recording",
		    PWARG_GSCB, gscb_pw_fb,
		    PWARG_CBARG_INT, PWFF_STOPREC, PWARG_LAST);
		break;
	default:
		pwidget_add(p, &fill_button, "Play",
		    PWARG_GSCB, gscb_pw_fb,
		    PWARG_CBARG_INT, PWFF_PLAY, PWARG_LAST);
		pwidget_add(p, &fill_button, "Record",
		    PWARG_GSCB, gscb_pw_fb,
		    PWARG_CBARG_INT, PWFF_REC, PWARG_LAST);
		pwidget_add(p, &fill_button, "Delete",
		    PWARG_GSCB, gscb_pw_fb,
		    PWARG_CBARG_INT, PWFF_CLR, PWARG_LAST);
		pwidget_add(p, (panel_for_id(PANEL_FBNEW) ?
		    &fill_checked : &fill_unchecked), "Create new",
		    PWARG_GSCB, gscb_pw_fb,
		    PWARG_CBARG_INT, PWFF_NEW, PWARG_LAST);
		pwidget_add(p, (panel_for_id(PANEL_FBCHO) ?
		    &fill_checked : &fill_unchecked), "Chooser",
		    PWARG_GSCB, gscb_pw_fb,
		    PWARG_CBARG_INT, PWFF_OPEN, PWARG_LAST);
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
	struct fvec lsph;

	if ((st.st_rf & RF_CAM) == 0 && panel_ready(p))
		return;

	vec_cart2sphere(&st.st_lv, &lsph);

	panel_set_content(p, "- Camera -\n"
	    "Position (%.2f,%.2f,%.2f)\n"
	    "Look (%.2f,%.2f,%.2f) (t=%.3f,p=%.3f)\n"
	    "Up (rot=%.3f,rev=%d)\n"
	    "Focus (%.2f,%.2f,%.2f)",
	    st.st_v.fv_x, st.st_v.fv_y, st.st_v.fv_z,
	    st.st_lv.fv_x, st.st_lv.fv_y, st.st_lv.fv_z,
	    lsph.fv_t, lsph.fv_p,
	    st.st_ur, st.st_urev,
	    focus.fv_x, focus.fv_y, focus.fv_z);
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
	    &fill_unchecked : &fill_checked, "Window Size",
	    PWARG_GSCB, gscb_pw_snap,
	    PWARG_CBARG_INT, 0,
	    PWARG_CBARG_INT2, 0, PWARG_LAST);

#define pwssres(w, h)						\
	pwidget_add(p, capture_usevirtual &&			\
	    virtwinv.iv_w == (w) && virtwinv.iv_h == (h) ?	\
	    &fill_checked : &fill_unchecked, #w "x" #h,		\
	    PWARG_GSCB, gscb_pw_snap,				\
	    PWARG_CBARG_INT, (w),				\
	    PWARG_CBARG_INT2, (h), PWARG_LAST)
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

		/* XXX save stat info in ds_open */
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
	panel_set_content(p, "(c) 2008 PSC\n%s", tmbuf);
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
		    &fill_checked : &fill_unchecked),
		    options[i].opt_name,
		    PWARG_GSCB, gscb_pw_opt,
		    PWARG_CBARG_INT, i, PWARG_LAST);
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
		    &fill_checked : &fill_unchecked), pinfo[i].pi_name,
		    PWARG_GSCB, gscb_pw_panel,
		    PWARG_CBARG_INT, i, PWARG_LAST);
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
		pwidget_add(p, &fill_xparent, "Panels",
		    PWARG_GSCB, gscb_pw_panel,
		    PWARG_CBARG_INT, baseconv(PANEL_PANELS) - 1, PWARG_LAST);
		pwidget_add(p, &fill_xparent, "Options",
		    PWARG_GSCB, gscb_pw_panel,
		    PWARG_CBARG_INT, baseconv(PANEL_OPTS) - 1, PWARG_LAST);
		pwidget_add(p, &fill_xparent, "Reorient",
		    PWARG_GSCB, gscb_pw_help,
		    PWARG_CBARG_INT, HF_REORIENT, PWARG_LAST);
		pwidget_add(p, &fill_xparent, "Update Data",
		    PWARG_GSCB, gscb_pw_help,
		    PWARG_CBARG_INT, HF_UPDATE, PWARG_LAST);
		if (nselnodes) {
			pwidget_add(p, &fill_xparent, "Clear Selnodes",
			    PWARG_GSCB, gscb_pw_help,
			    PWARG_CBARG_INT, HF_CLRSN, PWARG_LAST);
			pwidget_add(p, &fill_xparent, "Print Selnode IDs",
			    PWARG_GSCB, gscb_pw_help,
			    PWARG_CBARG_INT, HF_PRTSN, PWARG_LAST);
			pwidget_add(p, &fill_xparent, "Subselect Selnodes",
			    PWARG_GSCB, gscb_pw_help,
			    PWARG_CBARG_INT, HF_SUBSN, PWARG_LAST);
		}
	} else {
		pwidget_add(p, &fill_xparent,
		    (p->p_info->pi_stick == PSTICK_TL ||
		     p->p_info->pi_stick == PSTICK_BL) ?
		    ">> Help" : "Help <<",
		    PWARG_GSCB, gscb_pw_help,
		    PWARG_CBARG_INT, HF_SHOWHELP, PWARG_LAST);
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
	pwidget_group_start(p);
	for (i = 0; i < NVM; i++) {
		pwidget_add(p, (st.st_vmode == i ?
		    &fill_checked : &fill_unchecked),
		    vmodes[i].vm_name,
		    PWARG_GSCB, gscb_pw_vmode,
		    PWARG_CBARG_INT, i,
		    PWARG_GRP_CHECKED, st.st_vmode == i,
		    PWARG_LAST);
	}
	pwidget_group_end(p);
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
	pwidget_group_start(p);
	for (i = 0; i < NDM; i++) {
		if (dmodes[i].dm_name == NULL)
			continue;
		pwidget_add(p, (st.st_dmode == i ?
		    &fill_checked : &fill_unchecked),
		    dmodes[i].dm_name,
		    PWARG_GSCB, gscb_pw_dmode,
		    PWARG_CBARG_INT, i,
		    PWARG_GRP_CHECKED, st.st_dmode == i,
		    PWARG_LAST);
	}
	pwidget_group_end(p);
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
		    &fill_checked : &fill_unchecked), ssvclabels[i],
		    PWARG_GSCB, gscb_pw_ssvc,
		    PWARG_CBARG_INT, i, PWARG_LAST);
	}
	for (i = 0; i < NSSCNT; i++) {
		pwidget_add(p, (st.st_ssmode == i ?
		    &fill_checked : &fill_unchecked), ssmodelabels[i],
		    PWARG_GSCB, gscb_pw_ssmode,
		    PWARG_CBARG_INT, i, PWARG_LAST);
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
		    &fill_checked : &fill_unchecked), pipemodelabels[i],
		    PWARG_GSCB, gscb_pw_pipe,
		    PWARG_CBARG_INT, i, PWARG_LAST);
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
	pwidget_add(p, &fill_button, "x Space",
	    PWARG_GSCB, gscb_pw_wiadj,
	    PWARG_CBARG_INT, SWF_NSPX, PWARG_LAST);
	pwidget_add(p, &fill_button, "y Space",
	    PWARG_GSCB, gscb_pw_wiadj,
	    PWARG_CBARG_INT, SWF_NSPY, PWARG_LAST);
	pwidget_add(p, &fill_button, "z Space",
	    PWARG_GSCB, gscb_pw_wiadj,
	    PWARG_CBARG_INT, SWF_NSPZ, PWARG_LAST);
	pwidget_add(p, &fill_button, "x Offset",
	    PWARG_GSCB, gscb_pw_wiadj,
	    PWARG_CBARG_INT, SWF_OFFX, PWARG_LAST);
	pwidget_add(p, &fill_button, "y Offset",
	    PWARG_GSCB, gscb_pw_wiadj,
	    PWARG_CBARG_INT, SWF_OFFY, PWARG_LAST);
	pwidget_add(p, &fill_button, "z Offset",
	    PWARG_GSCB, gscb_pw_wiadj,
	    PWARG_CBARG_INT, SWF_OFFZ, PWARG_LAST);
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
	    &fill_checked : &fill_unchecked), "Recover",
	    PWARG_GSCB, gscb_pw_rt,
	    PWARG_CBARG_INT, SRF_RECOVER, PWARG_LAST);
	pwidget_add(p, (st.st_rtetype == RT_FATAL ?
	    &fill_checked : &fill_unchecked), "Fatal",
	    PWARG_GSCB, gscb_pw_rt,
	    PWARG_CBARG_INT, SRF_FATAL, PWARG_LAST);
	pwidget_add(p, (st.st_rtetype == RT_ROUTER ?
	    &fill_checked : &fill_unchecked), "Router",
	    PWARG_GSCB, gscb_pw_rt,
	    PWARG_CBARG_INT, SRF_ROUTER, PWARG_LAST);
	pwidget_add(p, &fill_nopanel, "", PWARG_LAST);
	pwidget_add(p, &fill_nopanel, "----- Pipe", PWARG_LAST);

	for (i = 0; i < NRTC / 2; i++)
		pwidget_add(p, &rtpipeclass[i].nc_fill,
		    rtpipeclass[i].nc_name, PWARG_LAST);

	pwidget_add(p, (st.st_rtepset == RPS_NEG ?
	    &fill_unchecked : &fill_unchecked), "Negative",
	    PWARG_GSCB, gscb_pw_rt,
	    PWARG_CBARG_INT, SRF_NEG, PWARG_LAST);
	pwidget_add(p, (st.st_rtepset == RPS_POS ?
	    &fill_checked : &fill_unchecked), "Positive",
	    PWARG_GSCB, gscb_pw_rt,
	    PWARG_CBARG_INT, SRF_POS, PWARG_LAST);
	pwidget_add(p, &fill_nopanel, "", PWARG_LAST);
	pwidget_add(p, &fill_nopanel, "", PWARG_LAST);
	pwidget_add(p, &fill_nopanel, "Legend ---", PWARG_LAST);

	for (; i < NRTC; i++)
		pwidget_add(p, &rtpipeclass[i].nc_fill,
		    rtpipeclass[i].nc_name, PWARG_LAST);

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
		pwidget_add(p, (j == keyh ?
		    &fill_checked : &fill_unchecked),
		    keyhtab[j].kh_name,
		    PWARG_GSCB, gscb_pw_keyh,
		    PWARG_CBARG_INT, j, PWARG_LAST);
	pwidget_endlist(p, 1);
}

#define PWDF_DIRSONLY (1<<0)

/*
 * Add pwidgets for all files in the given directory.
 * 'cur' is the currently selected file name.
 * 'dir' must point to a PATH_MAX-sized buffer.
 *
 * This must be called within a pwidget group.
 */
void
pwidgets_dir(struct panel *p, const char *dir, struct objlist *ol,
    char *cur, void *cb, int flags, int (*checkable)(const char *))
{
	int grp_member, isdir;
	char path[PATH_MAX];
	struct dirent *dent;
	struct fnent *fe;
	struct stat stb;
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
				    "Current Directory",
				    PWARG_SPRIO, 2,
				    PWARG_GSCB, gscb_pw_dir,
				    PWARG_CBARG_INT, 1,
				    PWARG_CBARG_PTR, cb,
				    PWARG_CBARG_PTR2, fe->fe_name,
				    PWARG_LAST);
			} else
#endif
			if (strcmp(dent->d_name, "..") == 0) {
				fe = obj_get(dent->d_name, ol);
				snprintf(fe->fe_name,
				    sizeof(fe->fe_name), "..");
				pwidget_add(p, &fill_xparent, "Up",
				    PWARG_SPRIO, 2,
				    PWARG_GSCB, gscb_pw_dir,
				    PWARG_CBARG_INT, 1,
				    PWARG_CBARG_PTR, cb,
				    PWARG_CBARG_PTR2, fe->fe_name,
				    PWARG_GRP_MEMBER, 0,
				    PWARG_LAST);
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
		if (!isdir && flags & PWDF_DIRSONLY)
			continue;

		/*
		 * Assign group membership: if we are browsing
		 * directories and a "checkable" callback was
		 * provided, use it; otherwise, assign based on
		 * whether it is a regular file or not.
		 */
		if (flags & PWDF_DIRSONLY && checkable)
			grp_member = checkable(path);
		else
			grp_member = !isdir;

		fe = obj_get(dent->d_name, ol);
		snprintf(fe->fe_name, sizeof(fe->fe_name), "%s%s",
		    dent->d_name, isdir ? "/" : "");
		pwidget_add(p, (strcmp(cur, path) ?
		    (isdir ? &fill_xparent : &fill_unchecked) : &fill_checked),
		    fe->fe_name,
		    PWARG_SPRIO, isdir,
		    PWARG_GSCB, gscb_pw_dir,
		    PWARG_CBARG_INT, isdir,
		    PWARG_CBARG_PTR, cb,
		    PWARG_CBARG_PTR2, fe->fe_name,
		    PWARG_GRP_MEMBER, grp_member,
		    PWARG_GRP_CHECKED, strcmp(cur, path) == 0, PWARG_LAST);
	}
	obj_batch_end(ol);
//	if (ret == -1)
//		err(1, "readdir %s", dir);
	closedir(dp);
}

int
dir_isreel(const char *fn)
{
	char path[PATH_MAX];
	struct stat stb;

	snprintf(path, sizeof(path), "%s/.isar", fn);
	if (stat(path, &stb) == -1) {
		if (errno != ENOENT)
			errx(1, "stat %s", path);
		return (0);
	}
	return (1);
}

int
dir_isdump(const char *fn)
{
	char path[PATH_MAX];
	struct stat stb;

	snprintf(path, sizeof(path), "%s/%s", fn, _PATH_ISDUMP);
	if (stat(path, &stb) == -1) {
		if (errno != ENOENT)
			errx(1, "stat %s", path);
		return (0);
	}
	return (1);
}

void
panel_refresh_fbcho(struct panel *p)
{
	if (panel_ready(p))
		return;

	panel_set_content(p, "- Flyby Chooser -\n%s", flyby_dir);
	pwidget_startlist(p);
	pwidget_group_start(p);
	pwidgets_dir(p, flyby_dir, &flyby_list, flyby_fn, flyby_set, 0,
	    NULL);
#if 0
	if (panel->p_nwidgets == 0)
		pwidget_add(p, &fill_white, FLYBY_DEFAULT, 0, );
#endif
	pwidget_group_end(p);
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
	pwidget_group_start(p);
	pwidgets_dir(p, dx_dir, &dxscript_list, dx_fn, dx_set, 0, NULL);
	pwidget_group_end(p);
	pwidget_sortlist(p, pwidget_cmp);
	pwidget_endlist(p, 2);

	if (p->p_nwidgets == 0)
		panel_add_content(p, "\nNo scripts available.");
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

		pwidget_group_start(p);
		pwidgets_dir(p, reel_browsedir, &reel_list, reel_dir,
		    reel_set, PWDF_DIRSONLY, dir_isreel);
		pwidget_group_end(p);
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
	pwidget_group_start(p);
	pwidget_add(p, (live ? &fill_checked : &fill_unchecked), "Live",
	    PWARG_SPRIO, 2,
	    PWARG_GSCB, gscb_pw_dscho,
	    PWARG_GRP_CHECKED, live, PWARG_LAST);
	pwidgets_dir(p, ds_browsedir, &ds_list, live ? "" : ds_dir,
	    ds_set, PWDF_DIRSONLY, dir_isdump);
	pwidget_group_end(p);
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

const char *vnmodes[] = {
	"A", "B", "C", "D"
};

void
panel_refresh_vnmode(struct panel *p)
{
	int i;

	if (panel_ready(p))
		return;

	panel_set_content(p, "- VNeighbor Mode -");
	pwidget_startlist(p);
	pwidget_group_start(p);
	for (i = 0; i < NVNM; i++)
		pwidget_add(p, (st.st_vnmode == i ? &fill_checked : &fill_unchecked),
		    vnmodes[i],
		    PWARG_GSCB, gscb_pw_vnmode,
		    PWARG_CBARG_INT, i,
		    PWARG_LAST);
	pwidget_group_end(p);
	pwidget_endlist(p, 2);
}

