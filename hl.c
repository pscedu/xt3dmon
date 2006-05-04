/* $Id$ */

#include "mon.h"

#include "flyby.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "yod.h"

void
nc_runall(void (*f)(struct fill *))
{
	size_t i;

	switch (st.st_dmode) {
	case DM_JOB:
		for (i = 0; i < job_list.ol_cur; i++)
			f(&job_list.ol_jobs[i]->j_fill);
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
			f(&yod_list.ol_yods[i]->y_fill);
		for (i = 0; i < NSC; i++)
			f(&statusclass[i].nc_fill);
		break;
	case DM_RTUNK:
		for (i = 0; i < NRTC; i++)
			f(&rtclass[i].nc_fill);
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
	}
}

struct fill *
nc_getfp(size_t nc)
{
	switch (st.st_dmode) {
	case DM_JOB:
		if (nc < NSC)
			return (&statusclass[nc].nc_fill);
		else if (nc - NSC < job_list.ol_cur)
			return (&job_list.ol_jobs[nc - NSC]->j_fill);
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
			return (&yod_list.ol_yods[nc - NSC]->y_fill);
		break;
	case DM_RTUNK:
		if (nc < NRTC)
			return (&rtclass[nc].nc_fill);
		break;
	case DM_BORG:
		return (&fill_borg);
	case DM_MATRIX:
		return (&fill_matrix);
	case DM_SAME:
		return (&fill_same);
	}
	return (NULL);
}

void
nc_runsn(void (*f)(struct fill *))
{
	struct selnode *sn;
	struct node *n;

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
			if (n->n_route.rt_err[RP_UNK][rt_type] != DV_NODATA)
				f(n->n_fillp);
			break;
		case DM_YOD:
			if (n->n_yod != NULL)
				f(&n->n_yod->y_fill);
			break;
		case DM_MATRIX:
		case DM_SAME:
		case DM_BORG:
			f(n->n_fillp);
			break;
		}
	}
}

void
hl_change(void)
{
	struct fill *fp;

	switch (st.st_hlnc) {
	case HL_ALL:
		nc_runall(fill_setopaque);
		break;
	case HL_NONE:
		nc_runall(fill_setxparent);
		break;
	case HL_SELDM:
		nc_runall(fill_setxparent);
		nc_runsn(fill_setopaque);
		break;
	default:
		fp = nc_getfp(st.st_hlnc);
		if (fp) {
			nc_runall(fill_setxparent);
			fill_setopaque(fp);
		}
		break;
	}
}
