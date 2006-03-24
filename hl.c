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
hl_setall(float val)
{
	size_t i;

	switch (st.st_dmode) {
	case DM_JOB:
		for (i = 0; i < job_list.ol_cur; i++)
			job_list.ol_jobs[i]->j_fill.f_a = val;
		for (i = 0; i < NSC; i++)
			statusclass[i].nc_fill.f_a = val;
		break;
	case DM_FAIL:
		for (i = 0; i < NFAILC; i++)
			failclass[i].nc_fill.f_a = val;
		break;
	case DM_TEMP:
		for (i = 0; i < NTEMPC; i++)
			tempclass[i].nc_fill.f_a = val;
		break;
	case DM_YOD:
		for (i = 0; i < yod_list.ol_cur; i++)
			yod_list.ol_yods[i]->y_fill.f_a = val;
		for (i = 0; i < NSC; i++)
			statusclass[i].nc_fill.f_a = val;
		break;
	case DM_BORG:
		fill_borg.f_a = val;
		break;
	case DM_MATRIX:
		fill_matrix.f_a = val;
		fill_matrix_reloaded.f_a = val;
		break;
	case DM_SAME:
		fill_same.f_a = val;
		break;
	case DM_RTUNK:
		for (i = 0; i < NRTC; i++)
			rtclass[i].nc_fill.f_a = val;
		break;
	}
}

void
hl_class(void)
{
	size_t nc;

	if (st.st_hlnc < 0)
		return;			/* XXX: fix it */
	nc = (size_t)st.st_hlnc;

	switch (st.st_dmode) {
	case DM_BORG:
	case DM_MATRIX:
	case DM_SAME:
		return;
	case DM_JOB:
		if (nc >= NSC + job_list.ol_cur)
			return;
		break;
	case DM_FAIL:
		if (nc >= NFAILC)
			return;
		break;
	case DM_TEMP:
		if (nc >= NTEMPC)
			return;
		break;
	case DM_YOD:
		if (nc >= NSC + yod_list.ol_cur)
			return;
		break;
	case DM_RTUNK:
		if (nc >= NRTC)
			return;
		break;
	}

	hl_setall(0.0f);
	switch (st.st_dmode) {
	case DM_JOB:
		if (nc < NSC)
			statusclass[nc].nc_fill.f_a = 1.0f;
		else
			job_list.ol_jobs[nc - NSC]->j_fill.f_a = 1.0f;
		break;
	case DM_YOD:
		if (nc < NSC)
			statusclass[nc].nc_fill.f_a = 1.0f;
		else
			yod_list.ol_yods[nc - NSC]->y_fill.f_a = 1.0f;
		break;
	case DM_TEMP:
		tempclass[nc].nc_fill.f_a = 1.0f;
		break;
	case DM_FAIL:
		failclass[nc].nc_fill.f_a = 1.0f;
		break;
	case DM_RTUNK:
		rtclass[nc].nc_fill.f_a = 1.0f;
		break;
	}
}

void
hl_seldm(void)
{
	struct selnode *sn;
	struct node *n;
	int found;

	found = 0;
	SLIST_FOREACH(sn, &selnodes, sn_next) {
		switch (st.st_dmode) {
		case DM_JOB:
			if (sn->sn_nodep->n_job != NULL)
				found = 1;
			break;
		case DM_FAIL:
		case DM_TEMP:
		case DM_RTUNK:
			found = 1;
			break;
		case DM_YOD:
			if (sn->sn_nodep->n_yod != NULL)
				found = 1;
			break;
		}
		if (found)
			break;
	}
	if (!found)
		return;

	hl_setall(0.0f);
	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;

		switch (st.st_dmode) {
		case DM_JOB:
			if (n->n_job != NULL)
				n->n_job->j_fill.f_a = 1.0f;
			break;
		case DM_FAIL:
			if (n->n_fails != DV_NODATA)
				failclass[roundclass(n->n_fails, FAIL_MIN,
				    FAIL_MAX, NFAILC)].nc_fill.f_a = 1.0f;
			break;
		case DM_TEMP:
			if (n->n_temp != DV_NODATA)
				tempclass[roundclass(n->n_temp, TEMP_MIN,
				    TEMP_MAX, NTEMPC)].nc_fill.f_a = 1.0f;
		case DM_RTUNK:
			if (n->n_route.rt_err[RP_UNK][rt_type] != DV_NODATA)
				rtclass[roundclass(n->n_route.rt_err[RP_UNK][rt_type],
				    0, rt_max.rt_err[RP_UNK][rt_type], NRTC)].nc_fill.f_a = 1.0f;
			break;
		case DM_YOD:
			if (n->n_yod != NULL)
				n->n_yod->y_fill.f_a = 1.0f;
			break;
		}
	}
}

void
hl_change(void)
{
	switch (st.st_hlnc) {
	case HL_ALL:
		hl_setall(1.0f);
		break;
	case HL_NONE:
		hl_setall(0.0f);
		break;
	case HL_SELDM:
		hl_seldm();
		break;
	default:
		hl_class();
		break;
	}
}
