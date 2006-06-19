/* $Id$ */

#include "mon.h"

#include "cam.h"
#include "capture.h"
#include "deusex.h"
#include "draw.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "panel.h"
#include "queue.h"
#include "server.h"
#include "state.h"
#include "tween.h"

#define FPS_TO_USEC(x)	(1e6 / (x))	/* Convert FPS to microseconds. */
#define GOVERN_FPS	30		/* FPS to govern at. */

long	 fps = 50;	/* Last FPS sample. */
long	 fps_cnt = 0;	/* Current FPS counter. */

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
	gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
	glMatrixMode(GL_MODELVIEW);
	st.st_rf |= RF_CAM;

	TAILQ_FOREACH(p, &panels, p_link)
		p->p_opts |= POPT_DIRTY;
}

void
gl_idleh_govern(void)
{
	static struct timeval gov_tv, tv, diff;

	gettimeofday(&tv, NULL);
	timersub(&tv, &gov_tv, &diff);
	if (diff.tv_sec * 1e6 + diff.tv_usec >= FPS_TO_USEC(GOVERN_FPS)) {
		fps_cnt++;
		gov_tv = tv;
		gl_run(glutPostRedisplay);
	}
}

void
gl_idleh_default(void)
{
	fps_cnt++;
	gl_run(glutPostRedisplay);
}

/* Per-application setup routine. */
void
gl_setup_core(void)
{
	glutTimerFunc(1, cocb_fps, 0);
	glutTimerFunc(1, cocb_clearstatus, 0); /* XXX: enable/disable when PANEL_STATUS? */
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
	glutIdleFunc(gl_idleh_default);

	if (server_mode) {
		glutDisplayFunc(serv_displayh);
		glutKeyboardFunc(gl_keyh_server);
	} else {
		if (stereo_mode)
			glutDisplayFunc(gl_displayh_stereo);
		else
			glutDisplayFunc(gl_displayh_default);
		glutKeyboardFunc(gl_keyh_default);
		glutMotionFunc(gl_motionh_default);
		glutMouseFunc(gl_mouseh_default);
		glutPassiveMotionFunc(gl_pasvmotionh_default);
		glutSpecialFunc(gl_spkeyh_default);
		glutMouseWheelFunc(gl_mwheel_default);
	}

	glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
//	glClearDepth(1.0f);
//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

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

void
gl_displayh_stereo(void)
{
	static struct frustum fr;
	static struct fvec oldfv;
	int frid, rf, newrf;

	if (flyby_mode)
		flyby_update();
	else if (st.st_opts & OP_DEUSEX)
		dx_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	rf = st.st_rf;
	if (rf) {
		st.st_rf = 0;
		rf = rebuild(rf);
	}

	frustum_init(&fr);

	switch (stereo_mode) {
	case STM_ACT:
//		glDrawBuffer(GL_BACK_RIGHT);
//		frid = (wid == WINID_LEFT) ? FRID_LEFT : FRID_RIGHT;
		break;
	case STM_PASV:
		gl_wid_update();
		frid = (wid == WINID_LEFT) ? FRID_LEFT : FRID_RIGHT;
		break;
	}

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

	newrf = st.st_rf;
	st.st_rf = rf;
	draw_scene();
	st.st_rf = newrf;

	/* XXX: capture frame */
	if (st.st_opts & OP_CAPTURE)
		capture_frame(capture_mode);
	if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();

	/* Restore camera position after stereo adjustment. */
	st.st_v = oldfv;
}

void
gl_displayh_default(void)
{
	int rf, newrf;

	if (flyby_mode)
		flyby_update();
	else if (st.st_opts & OP_DEUSEX)
		dx_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	rf = st.st_rf;
	if (rf) {
		st.st_rf = 0;
		rf = rebuild(rf);
	}

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
	st.st_rf = newrf;

	if (st.st_opts & OP_CAPTURE)
		capture_frame(capture_mode);
	if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();
}
