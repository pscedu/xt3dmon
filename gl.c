/* $Id$ */

#include "compat.h"
#include "mon.h"

#define FPS_TO_USEC(x)	(1e6 / x)	/* Convert FPS to microseconds. */
#define GOVERN_FPS	30		/* FPS governor. */

long			 fps = 50;	/* last fps sample */
long			 fps_cnt = 0;	/* current fps counter */

void
gl_reshapeh(int w, int h)
{
	struct panel *p;

	win_width = w;
	win_height = h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, win_width, win_height);
	gluPerspective(FOVY, ASPECT, 1, clip);
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
		glutPostRedisplay();
	}
}

void
gl_idleh_default(void)
{
	fps_cnt++;
	glutPostRedisplay();
}

void
gl_setup(void)
{
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	glutReshapeFunc(gl_reshapeh);
	glutKeyboardFunc(gl_keyh_default);
	glutSpecialFunc(gl_spkeyh_default);
	glutDisplayFunc(gl_displayhp);
	glutIdleFunc(gl_idleh_default);
	glutMouseFunc(gl_mouseh_default);
	glutMotionFunc(gl_motionh_default);
	glutPassiveMotionFunc(gl_pasvmotionh_default);

	glutTimerFunc(1, cocb_fps, 0);
	glutTimerFunc(1, cocb_clearstatus, 0); /* XXX: enable/disable when PANEL_STATUS? */

	tex_load();
}

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

__inline void
gl_setidleh(void)
{
	glutIdleFunc(st.st_opts & OP_GOVERN ?
	    gl_idleh_govern : gl_idleh_default);
}
