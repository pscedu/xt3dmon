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
#include <string.h>

#include "cam.h"
#include "capture.h"
#include "deusex.h"
#include "draw.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "panel.h"
#include "queue.h"
#include "select.h"
#include "server.h"
#include "state.h"
#include "tween.h"

#define FPS_TO_USEC(x)	(1e6 / (x))	/* Convert FPS to microseconds. */
#define GOVERN_FPS	32		/* FPS to govern at. */

long	 fps = 50;			/* Last FPS sample. */
long	 fps_cnt = 0;			/* Current FPS counter. */
int	 stereo_mode;
int	 visible;
int	 gl_drawhint = GL_DONT_CARE;

/*
 * Run a function in both GL stereo environments.
 * Anything which messes with the GL environment on
 * a per-window basis (as opposed to per-application)
 * should use this.
 */
__inline void
gl_run(void (*f)(void))
{
	if (stereo_mode == STM_PASV) {
		wid = WINID_LEFT;
		glutSetWindow(window_ids[wid]);
		(*f)();

		wid = WINID_RIGHT;
		glutSetWindow(window_ids[wid]);
		(*f)();
	} else
		(*f)();
}

/* Handle GL window resize events. */
void
gl_reshapeh(int w, int h)
{
	struct panel *p;

	/* Handle divide-by-zero when calculating aspect ratio. */
	if (h == 0)
		h++;

	winv.iv_w = w;
	winv.iv_h = h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, winv.iv_w, winv.iv_h);
	/* XXX stereo */
	gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
	glMatrixMode(GL_MODELVIEW);
	st.st_rf |= RF_CAM;

	TAILQ_FOREACH(p, &panels, p_link)
		p->p_opts |= POPT_DIRTY;
}


void
gl_visible(int state)
{
	visible = (state == GLUT_VISIBLE);
}

void
gl_idleh_govern(void)
{
	static struct timeval gov_tv, tmv, diff;

	gettimeofday(&tmv, NULL);
	timersub(&tmv, &gov_tv, &diff);
	if (diff.tv_sec * 1e6 + diff.tv_usec >= FPS_TO_USEC(GOVERN_FPS)) {
		fps_cnt++;
		gov_tv = tmv;
		if (stereo_mode == STM_PASV) {
			wid = WINID_MASTER;
			glutSetWindow(window_ids[wid]);
		}
		glutPostRedisplay();
	}
}

void
gl_idleh_default(void)
{
	fps_cnt++;
	if (!visible)
		usleep(100);
	if (stereo_mode == STM_PASV) {
		wid = WINID_MASTER;
		glutSetWindow(window_ids[wid]);
	}
	glutPostRedisplay();
}

/* Per-application setup routine. */
void
gl_setup_core(void)
{
	wid = WINID_MASTER;
	glutSetWindow(window_ids[wid]);

	glutIdleFunc(gl_idleh_default);
	if (server_mode)
		glutDisplayFunc(serv_displayh);
	else if (stereo_mode)
		glutDisplayFunc(gl_displayh_stereo);
	else
		glutDisplayFunc(gl_displayh_default);
	glutTimerFunc(1, cocb_fps, 0);
}

void
gl_displayh_null(void)
{
}

/* Per-window setup routine. */
void
gl_setup(void)
{
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
//	glDisable(GL_DITHER);

	glutReshapeFunc(gl_reshapeh);

	glutDisplayFunc(gl_displayh_null);
	if (server_mode) {
		glutKeyboardFunc(gl_keyh_server);
	} else {
		glutKeyboardFunc(gl_keyh_default);
		glutMotionFunc(gl_motionh_default);
		glutMouseFunc(gl_mouseh_default);
		glutPassiveMotionFunc(gl_pasvmotionh_default);
		glutSpecialFunc(gl_spkeyh_default);
		glutMouseWheelFunc(gl_mwheel_default);
		glutVisibilityFunc(gl_visible);
	}

	glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
//	glClearDepth(1.0f);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, gl_drawhint);

	tex_load();
}

__inline void
gl_setidleh(void)
{
	glutIdleFunc(st.st_opts & OP_GOVERN ?
	    gl_idleh_govern : gl_idleh_default);
}

/*
 * Synchronize which window we are dealing with (via wid)
 * with what GL is operating on, primarily used in window
 * event handlers.
 */
void
gl_wid_update(void)
{
	int i, win_id;

	win_id = glutGetWindow();
	for (i = 0; i < (int)(sizeof(window_ids) /
	    sizeof(window_ids[0])); i++)
		if (win_id == window_ids[i]) {
			wid = i;
			break;
		}
}

/*
 * Check if wireps need rebuilt, which should happen
 * when the camera moves from away from the center of
 * the current repetitions.
 */
__inline void
wired_update(void)
{
	if (st.st_x + clip > wi_repstart.fv_x + wi_repdim.fv_w ||
	    st.st_x - clip < wi_repstart.fv_x ||
	    st.st_y + clip > wi_repstart.fv_y + wi_repdim.fv_h ||
	    st.st_y - clip < wi_repstart.fv_y ||
	    st.st_z + clip > wi_repstart.fv_z + wi_repdim.fv_d ||
	    st.st_z - clip < wi_repstart.fv_z)
		st.st_rf |= RF_WIREP;
}

__inline void
gl_stereo_eye(int frid)
{
	static struct frustum fr;
	static struct fvec oldfv;

	frustum_init(&fr);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	frustum_calc(frid, &fr);
	glFrustum(fr.fr_left, fr.fr_right, fr.fr_bottom,
	    fr.fr_top, NEARCLIP, clip);

	glMatrixMode(GL_MODELVIEW);
	oldfv = st.st_v;
	switch (frid) {
	case WINID_LEFT:
		vec_subfrom(&fr.fr_stereov, &st.st_v);
		break;
	case WINID_RIGHT:
		vec_addto(&fr.fr_stereov, &st.st_v);
		break;
	}
	cam_look();
	draw_scene();

	if (st.st_opts & OP_CAPTURE)
		if ((st.st_opts & OP_CAPFBONLY) == 0 ||
		    flyby_mode == FBM_PLAY)
			capture_snap(capture_seqname(capture_mode),
			    capture_mode);
	glutSwapBuffers();

	/* Restore camera position after stereo adjustment. */
	st.st_v = oldfv;
}

void
gl_displayh_stereo(void)
{
	int t, rrf, lrf, lnewrf;

	if (dx_active)
		dx_update();
	if (flyby_mode)
		flyby_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	lrf = st.st_rf;
	rrf = 0;

	wid = WINID_LEFT;
	switch (stereo_mode) {
	case STM_ACT:
//		glDrawBuffer(GL_BACK_LEFT);
		break;
	case STM_PASV:
		glutSetWindow(window_ids[wid]);
		break;
	}

	if (lrf) {
		st.st_rf = 0;
		rrf = rf_deps(lrf) & RF_STEREO;
		lrf = rebuild(rf_deps(lrf));
	}
	lnewrf = st.st_rf;
	st.st_rf = lrf;
	gl_stereo_eye(FRID_LEFT);
	if (st.st_rf != lrf)
		errx(1, "internal error: draw_scene() modified "
		    "rebuild flags (%d<=>%d)", st.st_rf, lrf);

	wid = WINID_RIGHT;
	switch (stereo_mode) {
	case STM_ACT:
//		glDrawBuffer(GL_BACK_RIGHT);
		break;
	case STM_PASV:
		glutSetWindow(window_ids[wid]);
		break;
	}

	if (rrf) {
		st.st_rf = 0;

		/* Hack: pause to avoid node anim. */
		t = st.st_opts;
		st.st_opts |= OP_STOP;
		rrf = rebuild(rrf);
		st.st_opts = t;
	}
	st.st_rf = rrf;
	gl_stereo_eye(FRID_RIGHT);
	if (st.st_rf != rrf)
		errx(1, "internal error: draw_scene() modified "
		    "rebuild flags (%d<=>%d)", st.st_rf, rrf);

	st.st_rf = lnewrf;
}

void
gl_displayh_default(void)
{
	int rf, newrf;

	if (dx_active)
		dx_update();
	if (flyby_mode)
		flyby_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	rf = st.st_rf;
	if (rf) {
		st.st_rf = 0;
		rf = rebuild(rf_deps(rf));
	} else
		usleep(100);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (rf & RF_CAM) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
		glMatrixMode(GL_MODELVIEW);
		cam_look();
	}
	newrf = st.st_rf;
	st.st_rf = rf;
	draw_scene();
	if (st.st_rf != rf)
		errx(1, "internal error: draw_scene() modified "
		    "rebuild flags (%d<=>%d)", st.st_rf, rf);
	st.st_rf = newrf;

	if (st.st_opts & OP_CAPTURE)
		if ((st.st_opts & OP_CAPFBONLY) == 0 ||
		    flyby_mode == FBM_PLAY)
			capture_snap(capture_seqname(capture_mode),
			    capture_mode);
	glutSwapBuffers();
}

struct glname *
gl_select(int flags)
{
	struct glname *gn;
	struct fvec v;
	int dl[NGEOM];
	int i, nrecs;

	gn = NULL; /* gcc */
	switch (stereo_mode) {
	case STM_PASV:
		gl_wid_update();
		break;
	case STM_ACT:
		/* Assume last buffer drawn in. */
		break;
	}

	memset(dl, 0, sizeof(dl));

	sel_begin();
	draw_shadow_panels();
	nrecs = sel_end();
	if (nrecs && (gn = sel_process(nrecs, 0,
	    SPF_2D | flags)) != NULL || flags & SPF_2D)
		goto end;

//	if ((gn = sel_waslast(dl, flags)) != NULL)
//		goto end;

	switch (st.st_vmode) {
	case VM_VNEIGHBOR:
		gn = vneighbor_shadow(dl, flags);
		break;
	case VM_PHYS:
		gn = phys_shadow(dl, flags);
		break;
	case VM_WIRED:
		WIREP_FOREACH(&v)
			if ((gn = wi_shadow(dl, flags, &v)) != NULL)
				goto end;
		break;
	case VM_WIONE:
		gn = wi_shadow(dl, flags, &fv_zero);
		break;
	}
 end:
	for (i = 0; i < NGEOM; i++)
		if (dl[i])
			glDeleteLists(dl[i], 1);

	st.st_rf |= RF_CAM;

	if (gn == NULL)
		gscb_miss(NULL, flags);
	return (gn);
}

void
gl_setbg(void)
{
	glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
}
