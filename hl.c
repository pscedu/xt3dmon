/* $Id$ */

/*
 * Node class "highlighting" routines.
 *
 * The name is a white lie: we can do more than just
 * highlight.  These routines perform operations on
 * the various node classes, such as changing the
 * alpha on nodes with temperature 34-37C.
 */

#include "mon.h"

#include <err.h>

#include "cdefs.h"
#include "flyby.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "yod.h"

int
nc_gethlop(void (*f)(struct fill *))
{
	/* This highlight ops definition must match FBHLOP_* in flyby.h. */
	void (*hlops[])(struct fill *) = {
		fill_setxparent,
		fill_setopaque,
		fill_alphainc,
		fill_alphadec,
		fill_togglevis
	};
	size_t j;

	for (j = 0; j < NENTRIES(hlops); j++)
		if (hlops[j] == f)
			return (j);
	warnx("flyby: unknown fill operation");
	return (FBHLOP_UNKNOWN);
}

/*
 * Run a fill operation on all currently displayed node classes.
 */
void
nc_runall(void (*f)(struct fill *))
{
	size_t i;
	int hlop;

	switch (st.st_dmode) {
	case DM_JOB:
		for (i = 0; i < job_list.ol_cur; i++)
			f(&OLE(job_list, i, job)->j_fill);
		for (i = 0; i < NSC; i++)
			f(&statusclass[i].nc_fill);
		break;
	case DM_FAIL:
		for (i = 0; i < NFAILC; i++)
			f(&failclass[i].nc_fill);
		break;
	case DM_TEMP:
		for (i = 0; i < NTEMPC; i++)
			f(&tempclass[i].nc_fill);
		break;
	case DM_YOD:
		for (i = 0; i < yod_list.ol_cur; i++)
			f(&OLE(yod_list, i, yod)->y_fill);
		for (i = 0; i < NSC; i++)
			f(&statusclass[i].nc_fill);
		break;
	case DM_RTUNK:
		for (i = 0; i < NRTC; i++)
			f(&rtclass[i].nc_fill);
		f(&fill_rtesnd);
		f(&fill_rtercv);
		break;
	case DM_SEASTAR:
		for (i = 0; i < NSSC; i++)
			f(&ssclass[i].nc_fill);
		break;
	case DM_MATRIX:
		f(&fill_matrix);
		break;
	case DM_SAME:
		f(&fill_same);
		break;
	case DM_BORG:
		f(&fill_borg);
		break;
	case DM_LUSTRE:
		for (i = 0; i < NLUSTC; i++)
			f(&lustreclass[i].nc_fill);
		break;
	}

	hlnc_clean = 0;
	st.st_rf |= RF_CLUSTER | RF_SELNODE;

	if (flyby_mode != FBM_REC)
		return;
	hlop = nc_gethlop(f);
	if (hlop != FBHLOP_UNKNOWN)
		flyby_writehlnc(NC_ALL, hlop);
}

/*
 * Grab the fill for the given node class in the current display.
 * Do not modify the returned fill directly!  Such changes will
 * not be saved in flybys!
 */
struct fill *
nc_getfp(size_t nc)
{
	switch (st.st_dmode) {
	case DM_JOB:
		if (nc < NSC)
			return (&statusclass[nc].nc_fill);
		else if (nc - NSC < job_list.ol_cur)
			return (&OLE(job_list, nc - NSC, job)->j_fill);
		break;
	case DM_FAIL:
		if (nc < NFAILC)
			return (&failclass[nc].nc_fill);
		break;
	case DM_TEMP:
		if (nc < NTEMPC)
			return (&tempclass[nc].nc_fill);
		break;
	case DM_YOD:
		if (nc < NSC)
			return (&statusclass[nc].nc_fill);
		else if (nc - NSC < yod_list.ol_cur)
			return (&OLE(yod_list, nc - NSC, yod)->y_fill);
		break;
	case DM_RTUNK:
		if (nc < NRTC)
			return (&rtclass[nc].nc_fill);
		else if (nc == RTC_SND)
			return (&fill_rtesnd);
		else if (nc == RTC_RCV)
			return (&fill_rtercv);
		break;
	case DM_SEASTAR:
		if (nc < NSSC)
			return (&ssclass[nc].nc_fill);
		break;
	case DM_BORG:
		return (&fill_borg);
	case DM_MATRIX:
		return (&fill_matrix);
	case DM_SAME:
		return (&fill_same);
	case DM_LUSTRE:
		if (nc < NLUSTC)
			return (&lustreclass[nc].nc_fill);
		break;
	}
	return (NULL);
}

/*
 * Apply a fill operation on a node class.
 */
void
nc_apply(void (*f)(struct fill *), size_t nc)
{
	struct fill *fp;
	int hlop;

	fp = nc_getfp(nc);
	if (fp == NULL)
		return;

	f(fp);

	hlnc_clean = 0;
	st.st_rf |= RF_CLUSTER | RF_SELNODE;

	if (flyby_mode != FBM_REC)
		return;
	hlop = nc_gethlop(f);
	if (hlop != FBHLOP_UNKNOWN)
		flyby_writehlnc(nc, hlop);
}

/*
 * Run a fill operation on all node classes for which
 * any currently selected node are a member.
 */
void
nc_runsn(void (*f)(struct fill *))
{
	struct selnode *sn;
	struct node *n;
	int hlop;

	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;

		switch (st.st_dmode) {
		case DM_JOB:
			if (n->n_job != NULL)
				f(&n->n_job->j_fill);
			break;
		case DM_FAIL:
			if (n->n_fails != DV_NODATA)
				f(n->n_fillp);
			break;
		case DM_TEMP:
			if (n->n_temp != DV_NODATA)
				f(n->n_fillp);
			break;
		case DM_RTUNK:
			if (n->n_route.rt_err[RP_UNK][st.st_rtetype])
				f(n->n_fillp);
			/* XXX: neighbor detection */
			break;
		case DM_YOD:
			if (n->n_yod != NULL)
				f(&n->n_yod->y_fill);
			break;
		case DM_SEASTAR:
		case DM_LUSTRE:
		case DM_MATRIX:
		case DM_SAME:
		case DM_BORG:
			f(n->n_fillp);
			break;
		}
	}

	hlnc_clean = 0;
	st.st_rf |= RF_CLUSTER | RF_SELNODE;

	if (flyby_mode != FBM_REC)
		return;
	hlop = nc_gethlop(f);
	if (hlop != FBHLOP_UNKNOWN)
		flyby_writehlnc(NC_SELDM, hlop);
}

/* Select the given node class -- hide all others. */
int
nc_set(int nc)
{
	int ret;

	ret = 1;
	switch (nc) {
	case NC_ALL:
		nc_runall(fill_setopaque);
		break;
	case NC_NONE:
		/* XXX ? */
		break;
	case NC_SELDM:
		nc_runall(fill_setxparent);
		nc_runsn(fill_setopaque);
		break;
	default:
		if (nc_getfp(nc) != NULL) {
			nc_runall(fill_setxparent);
			nc_apply(fill_setopaque, nc);
			ret = 0;
		}
		break;
	}
	return (ret);
}
