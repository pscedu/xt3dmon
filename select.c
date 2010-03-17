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

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "cam.h"
#include "capture.h"
#include "deusex.h"
#include "ds.h"
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

struct objlist glname_list = { NULL, 0, 0, 0, 0, GINCR, sizeof(struct glname), glname_eq };

GLuint	 selbuf[2048];

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
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
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

struct glname *
sel_process(int nrecs, int rank, int flags)
{
	struct glname *gn, *gn2d;
	struct selrec *sr;
	int i, start;
	GLuint *p;

	if (nrecs < 0) {
		warnx("hit record overflow: bump select buffer size");
		return (NULL);
	}

	for (i = 0, p = selbuf; i < nrecs; i++, p += 3 + p[SBI_LEN])
		if (p[SBI_LEN] != 1)
			errx(1, "selection buffer contains partial elements");

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
					return (NULL);
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
		return (NULL);

	if (gn == NULL)
		return (NULL);

	if (gn->gn_cb != NULL && (flags & SPF_LOOKUP) == 0)
		gn->gn_cb(gn, flags & ~SPF_2D);
	return (gn);
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
gsn_get(int flags, const struct fvec *offv, gscb_t cb,
    int arg_int, int arg_int2, void *arg_ptr, void *arg_ptr2)
{
	struct glname *gn;
	unsigned int cur = glname_list.ol_tcur + 100;

	gn = obj_get(&cur, &glname_list);
	gn->gn_name = cur;
	gn->gn_cb = cb;
	gn->gn_arg_int = arg_int;
	gn->gn_arg_int2 = arg_int2;
	gn->gn_arg_ptr = arg_ptr;
	gn->gn_arg_ptr2 = arg_ptr2;
	gn->gn_arg_ptr3 = NULL;
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
	int id = gn->gn_arg_int;

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
	int nid = gn->gn_arg_int;
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
	}
}

void
gscb_pw_hlnc(struct glname *gn, int flags)
{
	int nc = gn->gn_arg_int;
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
	int dm = gn->gn_arg_int, nc = gn->gn_arg_int2;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
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
	int opt = gn->gn_arg_int;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0)
		opt_flip(1 << opt);
}

void
gscb_pw_panel(struct glname *gn, int flags)
{
	int pid = gn->gn_arg_int;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0)
		panel_toggle(1 << pid);
}

void
gscb_pw_vmode(struct glname *gn, int flags)
{
	int vm = gn->gn_arg_int;

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
	int opt = gn->gn_arg_int;
	struct node *n, **np;
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
			tween_push();
			cam_bird(st.st_vmode);
			tween_pop();
			break;
		case HF_PRTSN:
			SLIST_FOREACH(sn, &selnodes, sn_next)
				printf("%d%s", sn->sn_nodep->n_nid,
				    SLIST_NEXT(sn, sn_next) ? "," : "\n");
			break;
		case HF_SUBSN:
			NODE_FOREACH_WI(n, np)
				n->n_flags &= ~NF_SUBSET;
			SLIST_FOREACH(sn, &selnodes, sn_next)
				sn->sn_nodep->n_flags |= NF_SUBSET;
			st.st_opts |= OP_SUBSET;
			st.st_rf |= RF_CLUSTER | RF_SELNODE;
			break;
		case HF_SUBTOG:
			SLIST_FOREACH(sn, &selnodes, sn_next)
				sn->sn_nodep->n_flags ^= NF_SUBSET;
			if (st.st_opts & OP_SUBSET)
				st.st_rf |= RF_CLUSTER | RF_SELNODE;
			break;
		case HF_SUBCL:
			NODE_FOREACH_WI(n, np)
				n->n_flags &= ~NF_SUBSET;
			if (st.st_opts & OP_SUBSET) {
				st.st_rf |= RF_CLUSTER | RF_SELNODE;
				st.st_opts &= ~OP_SUBSET;
			}
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
	int dm = gn->gn_arg_int;

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
	int vc = gn->gn_arg_int;

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
	int m = gn->gn_arg_int;

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
	int m = gn->gn_arg_int;

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
gscb_pw_fb(struct glname *gn, int flags)
{
	int i = gn->gn_arg_int;

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
	int swf = gn->gn_arg_int;
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
		    RF_SELNODE | RF_VMODE;

		/*
		 * Reorient camera by revolving ever so slightly.
		 */
		tween_push();
		cam_revolvefocus(0.0, 0.001);
		tween_pop();
	}
}

void
gscb_pw_rt(struct glname *gn, int flags)
{
	int srf = gn->gn_arg_int;

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
	int kh = gn->gn_arg_int;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		keyh = kh;
		glutSpecialFunc(keyhtab[keyh].kh_spkeyh);
	}
}

void
gscb_pw_dscho(__unused struct glname *gn, int flags)
{
	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0)
		ds_setlive();
}

/*
 * Callback for directory browser selection.
 *
 * We call the callback specified in the glname pointer argument.  For
 * directories, that routine checks whether selection is acceptable for
 * the new directory and returns a pointer to write the new path to if
 * it is.
 */
void
gscb_pw_dir(struct glname *gn, int flags)
{
	char *(*cb)(const char *, int) = gn->gn_arg_ptr;
	char *new = gn->gn_arg_ptr2;
	int isdir = gn->gn_arg_int;
	char *p, *t;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		if (isdir && (p = strchr(new, '/')) != NULL)
			*p = '\0';
		p = cb(new, isdir ? CHF_DIR : 0);
		if (isdir && p != NULL) {
			/*
			 * OK, a directory was chosen and the callback
			 * informed us that it is OK to change to that
			 * directory.
			 *
			 * If we go up the file hierarchy past '.' of
			 * the current working directory, change to the
			 * current working directory.  Otherwise, strip
			 * a component of the path off.
			 */
			if (strcmp(new, "..") == 0) {
				if ((t = strrchr(p, '/')) == NULL) {
					if (getcwd(p, PATH_MAX) == NULL)
						err(1, "getcwd");
				} else if (t != p)
					*t = '\0';
			} else if (strcmp(new, ".") != 0) {
				/*
				 * Descending into a subdirectory:
				 * tack new component onto path.
				 *
				 * We hack around dumb snprintf
				 * implementations by duplicating.
				 */
				if ((t = strdup(p)) == NULL)
					err(1, "strdup");
				snprintf(p, PATH_MAX, "%s/%s", t, new);
				free(t);
			}
		}
	}
}

void
gscb_pw_snap(__unused struct glname *gn, int flags)
{
	int w = gn->gn_arg_int;
	int h = gn->gn_arg_int2;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		if (w) {
			capture_usevirtual = 1;
			virtwinv.iv_w = w;
			virtwinv.iv_h = h;
		} else
			capture_usevirtual = 0;
		panel_rebuild(PANEL_SS);
	}
}

void
gscb_pw_vnmode(struct glname *gn, int flags)
{
	int vnm = gn->gn_arg_int;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		st.st_vnmode = vnm;
		st.st_rf |= RF_VMODE;
		panel_rebuild(PANEL_VNMODE);
	}
}

void
gscb_pw_pipedim(struct glname *gn, int flags)
{
	int pd = gn->gn_arg_int;

	if (flags & SPF_PROBE)
		cursor_set(GLUT_CURSOR_INFO);
	else if (flags == 0) {
		st.st_pipedim ^= pd;
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
	}
}
