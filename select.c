/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "cam.h"
#include "deusex.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "reel.h"
#include "select.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"
#include "util.h"

#define GINCR	10

int glname_eq(const void *, const void *);

struct objlist glname_list = { { NULL }, 0, 0, 0, 0, GINCR, sizeof(struct glname), glname_eq };

GLuint	 selbuf[1000];

int
glname_eq(const void *elem, const void *arg)
{
	return (((struct glname *)elem)->gn_name == *(unsigned int *)arg);
}

void
sel_begin(void)
{
	GLint viewport[4];
	struct frustum fr;
	int frid;

	glSelectBuffer(sizeof(selbuf) / sizeof(selbuf[0]), selbuf);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glRenderMode(GL_SELECT);
	glInitNames();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPickMatrix(mousev.iv_x, winv.iv_h - mousev.iv_y, 1, 1, viewport);

	switch (stereo_mode) {
	case STM_NONE:
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip); /* XXX wrong */
		break;
	case STM_PASV:
	case STM_ACT:
		frustum_init(&fr);

//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (stereo_mode == STM_PASV)
			frid = (wid == WINID_LEFT) ? FRID_LEFT : FRID_RIGHT ;
		else
			frid = FRID_LEFT; /* XXX: assume last buffer drawn to? */

		frustum_calc(frid, &fr);
		glFrustum(fr.fr_left, fr.fr_right, fr.fr_bottom,
		    fr.fr_top, NEARCLIP, clip);

		//vec_addto(&fr.fr_stereov, &st.st_v);
		break;
	}

	glMatrixMode(GL_MODELVIEW);

	obj_batch_start(&glname_list);
}

#define SBI_LEN		0

/*
 * Each GL selecction record consists of the following:
 *	- the number of names in this stack frame,
 *	- the minimum and maximum depths of all
 *	  vertices hit since the previous event, and
 *	- stack contents, bottom first, of which we assume
 *	  consists of only one element.
 */
struct selrec {
	unsigned int sr_names;
	unsigned int sr_minu;
	unsigned int sr_minv;
	unsigned int sr_name;
};

int
selrec_cmp(const void *a, const void *b)
{
	int retval;

	retval = CMP(((struct selrec *)a)->sr_minu,
	    ((struct selrec *)b)->sr_minu);
	if (retval == 0)
		retval = CMP(((struct selrec *)a)->sr_minv,
		    ((struct selrec *)b)->sr_minv);
	return (retval);
}

int
sel_process(int nrecs, int rank, int flags)
{
	struct glname *gn, *gn2d;
	struct selrec *sr;
	int i, start;
	GLuint *p;

	/* XXX:  sanity-check nrecs? */
	for (i = 0, p = selbuf; i < nrecs; i++, p += 3 + p[SBI_LEN])
		if (p[SBI_LEN] != 1) {
			warnx("selection buffer contains uneven elements");
			return (SP_MISS);
		}

	qsort(selbuf, nrecs, sizeof(*sr), selrec_cmp);

	/*
	 * 2D objects always get placed in the selection buffer,
	 * so the workaround is to pass the parameters of where
	 * they have been drawn on the screen and explicitly check
	 * that the cursor is within the bounds.
	 */
	start = 0;
	if ((flags & SPF_2D) == 0) {
		/* Skip 2D records. */
		for (i = 0, sr = (struct selrec *)selbuf; i < nrecs;
		    i++, sr++) {
			gn = obj_get(&sr->sr_name, &glname_list);
			if ((gn->gn_flags & GNF_2D) == 0)
				break;
		}
		start = i;
	}

	/*
	 * Find the selection record with the given rank.
	 */
	gn = NULL; /* gcc */
	gn2d = NULL;
	sr = &((struct selrec *)selbuf)[start];
	for (i = start; i < nrecs; i++, sr++) {
		gn = obj_get(&sr->sr_name, &glname_list);

		if (flags & SPF_2D) {
			/* 2D records always come first. */
			if ((gn->gn_flags & GNF_2D) == 0) {
				/*
				 * There are now no more 2D records left,
				 * so if we didn't find one, return failure;
				 * else, process the best found.
				 */
				if (gn2d == NULL)
					return (SP_MISS);
				else
					break;
			}
			if (mousev.iv_x >= gn->gn_u &&
			    mousev.iv_x <= gn->gn_u + gn->gn_w &&
			    mousev.iv_y >= winv.iv_h - gn->gn_v &&
			    mousev.iv_y <= winv.iv_h - gn->gn_v + gn->gn_h) {
				/* Save if smaller (better). */
				if (gn2d == NULL || gn->gn_w * gn->gn_h <
				    gn2d->gn_w * gn2d->gn_h)
					gn2d = gn;
			}
		} else if (rank-- == 0)
				break;
	}

	if (flags & SPF_2D)
		gn = gn2d;
	else if (i == nrecs)
		return (SP_MISS);

	if (gn == NULL)
		return (SP_MISS);

	if (gn->gn_cb != NULL)
		gn->gn_cb(gn, flags & ~SPF_2D);

	if (gn->gn_id == SP_MISS)
		errx(1, "selected object uses reserved ID %d", SP_MISS);

	return (gn->gn_id);
}

int
sel_end(void)
{
	obj_batch_end(&glname_list);

	glMatrixMode(GL_PROJECTION);
	glFlush();

	return (glRenderMode(GL_RENDER));
}

/*
 * Obtain a unique GL selection name.
 * Batches of these must be called within a
 * obj_batch_start/obj_batch_end combo.
 */
unsigned int
gsn_get(int id, void (*cb)(struct glname *, int), int flags,
    const struct fvec *offv)
{
	struct glname *gn;
	unsigned int cur = glname_list.ol_tcur + 100;

	gn = obj_get(&cur, &glname_list);
	gn->gn_name = cur;
	gn->gn_id = id;
	gn->gn_cb = cb;
	gn->gn_flags = flags;
	gn->gn_offv = *offv;
	return (cur);
}

/*
 * GL selection callbacks.
 */
void
gscb_miss(__unused struct glname *gn, int flags)
{
	if (flags & SPF_PROBE) {
		cursor_set(GLUT_CURSOR_CYCLE);
		exthelp = 0;
	}
}

void
gscb_panel(struct glname *gn, int flags)
{
	int id = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_RIGHT_ARROW);
	else if (flags == 0) {
		/* GL giving us crap. */
		if ((panel_mobile = panel_for_id(id)) != NULL) {
			glutMotionFunc(gl_motionh_panel);
			panel_mobile->p_opts |= POPT_MOBILE;
		}
	}
}

void
gscb_node(struct glname *gn, int flags)
{
	int nid = gn->gn_id;
	struct node *n;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		if ((n = node_for_nid(nid)) == NULL)
			return;

		/* spkey = glutGetModifiers(); */
		switch (spkey) {
		case GLUT_ACTIVE_SHIFT:
			sn_toggle(n, &gn->gn_offv);
			break;
		default:
			sn_set(n, &gn->gn_offv);
			break;
		}
		if (SLIST_EMPTY(&selnodes))
			panel_hide(PANEL_NINFO);
		else
			panel_show(PANEL_NINFO);
	}
}

void
gscb_pw_hlnc(struct glname *gn, int flags)
{
	int nc = gn->gn_id;
	void (*f)(struct fill *);

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags & (SPF_SQUIRE | SPF_DESQUIRE)) {
		f = (flags & SPF_SQUIRE) ?
		    fill_alphainc : fill_alphadec;
		if (nc == NC_ALL)
			nc_runall(f);
		else if (nc >= 0)
			nc_apply(f, nc);
		st.st_rf |= RF_CLUSTER;
	} else if (flags == 0) {
		spkey = glutGetModifiers();

		switch (nc) {
		case NC_ALL:
			nc_runall(fill_setopaque);
			break;
		default:
			if (nc_getfp(nc) != NULL) {
				if ((spkey & GLUT_ACTIVE_SHIFT) == 0)
					nc_runall(fill_setxparent);
				nc_apply(fill_togglevis, nc);
			}
			break;
		}
	}
}

void
gscb_pw_dmnc(struct glname *gn, int flags)
{
	int dm, nc, spwdn = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		SPWDN_SPLIT(spwdn, dm, nc);
		spkey = glutGetModifiers();

		if (st.st_dmode != dm) {
			st.st_dmode = dm;
			st.st_rf |= RF_DMODE;
		}

		/* nc will not be NC_* */
		if (nc_getfp(nc) != NULL) {
			if ((spkey & GLUT_ACTIVE_SHIFT) == 0)
				nc_runall(fill_setxparent);
			nc_apply(fill_togglevis, nc);
		}
	}
}

void
gscb_pw_opt(struct glname *gn, int flags)
{
	int opt = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0)
		opt_flip(1 << opt);
}

void
gscb_pw_panel(struct glname *gn, int flags)
{
	int pid = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0)
		panel_toggle(1 << pid);
}

void
gscb_pw_vmode(struct glname *gn, int flags)
{
	int vm = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		st.st_vmode = vm;
		st.st_rf |= RF_VMODE;
	}
}

void
gscb_pw_help(struct glname *gn, int flags)
{
	int opt = gn->gn_id;
	struct selnode *sn;

	if (flags & SPF_PROBE) {
		cursor_set(GLUT_CURSOR_INFO);
		exthelp = 1;
	} else if (flags == 0) {
		switch (opt) {
		case HF_SHOWHELP:
			exthelp = 1;
			break;
		case HF_HIDEHELP:
			exthelp = 0;
			break;
		case HF_CLRSN:
			sn_clear();
			break;
		case HF_REORIENT:
			tween_push(TWF_POS | TWF_LOOK | TWF_UP);
			cam_bird();
			tween_pop(TWF_POS | TWF_LOOK | TWF_UP);
			break;
		case HF_PRTSN:
			SLIST_FOREACH(sn, &selnodes, sn_next)
				printf("%d%s", sn->sn_nodep->n_nid,
				    SLIST_NEXT(sn, sn_next) ? "," : "\n");
			break;
		case HF_UPDATE:
			st.st_rf |= RF_DATASRC;
			break;
		}
	}
}

void
gscb_pw_dmode(struct glname *gn, int flags)
{
	int dm = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		if (dm == DM_SEASTAR ||
		    st.st_dmode == DM_SEASTAR)
			geom_setall(dm == DM_SEASTAR ?
			    GEOM_SPHERE : GEOM_CUBE);

		st.st_dmode = dm;
		st.st_rf |= RF_DMODE;
	}
}

void
gscb_pw_ssvc(struct glname *gn, int flags)
{
	int vc = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		st.st_ssvc = vc;
		st.st_rf |= RF_DMODE;
	}
}

void
gscb_pw_ssmode(struct glname *gn, int flags)
{
	int m = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		st.st_ssmode = m;
		st.st_rf |= RF_DMODE;
	}
}

void
gscb_pw_pipe(struct glname *gn, int flags)
{
	int m = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		st.st_pipemode = m;
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
		if ((st.st_opts & (OP_PIPES | OP_SELPIPES)) == 0)
			opt_enable(OP_PIPES);
	}
}

void
gscb_pw_reel(struct glname *gn, int flags)
{
	int i = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		snprintf(reel_fn, sizeof(reel_fn), "%s",
		    reel_list.ol_reels[i]->rl_dirname);
		st.st_rf |= RF_REEL;
	}
}

void
gscb_pw_fbcho(struct glname *gn, int flags)
{
	int i = gn->gn_id;
	struct panel *p;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		snprintf(flyby_name, sizeof(flyby_name), "%s",
		    flyby_list.ol_fnents[i]->fe_name);
		if ((p = panel_for_id(PANEL_FLYBY)) != NULL)
			p->p_opts |= POPT_REFRESH;
	}
}

void
gscb_pw_fb(struct glname *gn, int flags)
{
	int i = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		switch (i) {
		case PWFF_STOPREC:
			flyby_end();
			break;
		case PWFF_PLAY:
			flyby_begin(FBM_PLAY);
			break;
		case PWFF_REC:
			flyby_begin(FBM_REC);
			break;
		case PWFF_CLR:
			flyby_clear();
			break;
		case PWFF_OPEN:
			panel_toggle(PANEL_FBCHO);
			break;
		case PWFF_NEW:
			panel_toggle(PANEL_FBNEW);
			break;
		}
	}
}

void
gscb_pw_wiadj(struct glname *gn, int flags)
{
	int swf = gn->gn_id;
	int adj = 1;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if ((flags & (SPF_SQUIRE | SPF_DESQUIRE))) {
		if (flags & SPF_DESQUIRE)
			adj *= -1;
		switch (swf) {
		case SWF_NSPX:
			st.st_winsp.iv_x += adj;
			break;
		case SWF_NSPY:
			st.st_winsp.iv_y += adj;
			break;
		case SWF_NSPZ:
			st.st_winsp.iv_z += adj;
			break;
		case SWF_OFFX:
			st.st_wioff.iv_x += adj;
			break;
		case SWF_OFFY:
			st.st_wioff.iv_y += adj;
			break;
		case SWF_OFFZ:
			st.st_wioff.iv_z += adj;
			break;
		}
		st.st_rf |= RF_CLUSTER | RF_GROUND | RF_CAM |
		    RF_SELNODE | RF_NODESWIV;

		tween_push(TWF_POS | TWF_LOOK | TWF_UP);
		cam_revolvefocus(0.0, 0.001, REVT_LKAVG);
		tween_pop(TWF_POS | TWF_LOOK | TWF_UP);
	}
}

void
gscb_pw_rt(struct glname *gn, int flags)
{
	int srf = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		switch (srf) {
		case SRF_POS:
			st.st_rtepset = RPS_POS;
			break;
		case SRF_NEG:
			st.st_rtepset = RPS_NEG;
			break;
		case SRF_RECOVER:
			st.st_rtetype = RT_RECOVER;
			break;
		case SRF_FATAL:
			st.st_rtetype = RT_FATAL;
			break;
		case SRF_ROUTER:
			st.st_rtetype = RT_ROUTER;
			break;
		}
		st.st_rf |= RF_CLUSTER | RF_DMODE;
	}
}

void
gscb_pw_keyh(struct glname *gn, int flags)
{
	int kh = gn->gn_id;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		keyh = kh;
		glutSpecialFunc(keyhtab[keyh].kh_spkeyh);
	}
}

void
gscb_pw_dxcho(struct glname *gn, int flags)
{
	int i = gn->gn_id;
	struct panel *p;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		dx_setfn(dxscript_list.ol_fnents[i]->fe_name);
		if ((p = panel_for_id(PANEL_DXCHO)) != NULL)
			p->p_opts |= POPT_REFRESH;
	}
}
