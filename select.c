/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>

#include "cdefs.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "node.h"
#include "nodeclass.h"
#include "objlist.h"
#include "panel.h"
#include "select.h"
#include "selnode.h"
#include "state.h"
#include "util.h"

GLuint	 selbuf[1000];
int	 gl_cursor;

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

	if (flags & SPF_PROBE) {
		if ((gn->gn_flags & GNF_NOCUR) == 0 &&
		    gl_cursor != gn->gn_cursor) {
			glutSetCursor(gn->gn_cursor);
			gl_cursor = gn->gn_cursor;
		}
	} else if (gn->gn_cb != NULL)
		gn->gn_cb(gn->gn_id);

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
gsn_get(int id, void (*cb)(int), int flags, int cursor)
{
	struct glname *gn;
	unsigned int cur = glname_list.ol_tcur + 100;

	gn = obj_get(&cur, &glname_list);
	gn->gn_name = cur;
	gn->gn_id = id;
	gn->gn_cb = cb;
	gn->gn_flags = flags;
	gn->gn_cursor = cursor;
	return (cur);
}

/*
 * GL selection callbacks.
 */
void
gscb_panel(int id)
{
	/* GL giving us crap. */
	if ((panel_mobile = panel_for_id(id)) != NULL) {
		glutMotionFunc(gl_motionh_panel);
		panel_mobile->p_opts |= POPT_MOBILE;
	}
}

void
gscb_node(int nid)
{
	struct node *n;

	if ((n = node_for_nid(nid)) == NULL)
		return;

	/* spkey = glutGetModifiers(); */
	switch (spkey) {
	case GLUT_ACTIVE_SHIFT:
		sn_add(n);
		break;
	case GLUT_ACTIVE_CTRL:
		sn_del(n);
		break;
	case 0:
		sn_set(n);
		break;
	}
	if (SLIST_EMPTY(&selnodes))
		panel_hide(PANEL_NINFO);
	else
		panel_show(PANEL_NINFO);
	return;
}

void
gscb_pw_hlnc(int nc)
{
	st.st_hlnc = nc;
	st.st_rf |= RF_HLNC;
}

void
gscb_pw_opt(int opt)
{
	opt_flip(1 << opt);
}

void
gscb_pw_panel(int pid)
{
	panel_toggle(1 << pid);
}

void
gscb_pw_vmode(int vm)
{
	st.st_vmode = vm;
	st.st_rf |= RF_CLUSTER | RF_CAM | RF_GROUND | RF_SELNODE;
}

void
gscb_pw_help(int opt)
{
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
	}
}

void
gscb_pw_dmode(int dm)
{
	st.st_dmode = dm;
	st.st_rf |= RF_DMODE;
}
