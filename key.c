/* $Id$ */

#include <errno.h>
#include <unistd.h>

#include "buf.h"
#include "cdefs.h"
#include "mon.h"

#define TRANS_INC	0.10

struct uinput uinp;

void
spkeyh_null(__unused int key, __unused int u, __unused int v)
{
}

void
spkeyh_actflyby(__unused int key, __unused int u, __unused int v)
{
	end_flyby();
	glutKeyboardFunc(keyh_default);
	glutSpecialFunc(spkeyh_default);
	glutMotionFunc(m_activeh_default);
	glutPassiveMotionFunc(m_passiveh_default);
}

void
keyh_actflyby(__unused unsigned char key, __unused int u, __unused int v)
{
	end_flyby();
	glutKeyboardFunc(keyh_default);
	glutSpecialFunc(spkeyh_default);
	glutMotionFunc(m_activeh_default);
	glutPassiveMotionFunc(m_passiveh_default);
}

void
keyh_flyby(unsigned char key, __unused int u, __unused int v)
{
	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'c':
		unlink(_PATH_FLYBY);
		errno = 0;
		break;
	case 'q':
		if (build_flyby)
			end_flyby();
		else if (!active_flyby)
			begin_flyby('w');
		break;
	case 'p':
		if (active_flyby)
			end_flyby();
		else if (!build_flyby) {
			begin_flyby('r');
			glutMotionFunc(m_activeh_null);
			glutPassiveMotionFunc(m_passiveh_null);
			glutKeyboardFunc(keyh_actflyby);
			glutSpecialFunc(spkeyh_actflyby);
		}
		break;
	case 'l':
		st.st_opts ^= OP_LOOPFLYBY;
		refresh_state(st.st_opts ^ OP_LOOPFLYBY);
		break;
	}
}

void
keyh_uinput(unsigned char key, __unused int u, __unused int v)
{
	int opts;

	uinp.uinp_opts |= UINPO_DIRTY;
	switch (key) {
	case 13: /* enter */
		glutKeyboardFunc(keyh_default);
		opts = uinp.uinp_opts;
		uinp.uinp_callback();
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		if ((opts & UINPO_LINGER) == 0)
			panel_remove(uinp.uinp_panel);
		break;
	case 27: /* escape */
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		glutKeyboardFunc(keyh_default);
		panel_remove(uinp.uinp_panel);
		break;
	case 8: /* backspace */
		if (strlen(buf_get(&uinp.uinp_buf)) > 0) {
			buf_chop(&uinp.uinp_buf);
			buf_chop(&uinp.uinp_buf);
			buf_append(&uinp.uinp_buf, '\0');
		}
		break;
	default:
		buf_chop(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, key);
		buf_append(&uinp.uinp_buf, '\0');
		break;
	}
}

void
keyh_panel(unsigned char key, __unused int u, __unused int v)
{
	int j;

	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'a':
		for (j = 0; j < NPANELS; j++)
			if ((pinfo[j].pi_opts & PF_UINP) == 0)
				panel_toggle(1 << j);
		break;
	case 'c':
		panel_toggle(PANEL_CMD);
		break;
	case 'F':
		panel_toggle(PANEL_FLYBY);
		break;
	case 'f':
		panel_toggle(PANEL_FPS);
		break;
	case 'g':
		panel_toggle(PANEL_GOTO);
		break;
	case 'l':
		panel_toggle(PANEL_LEGEND);
		break;
	case 'n':
		panel_toggle(PANEL_NINFO);
		break;
	case 'p':
		panel_toggle(PANEL_POS);
		break;
	case 's':
		panel_toggle(PANEL_SS);
		break;
	}
}

void
keyh_mode(unsigned char key, __unused int u, __unused int v)
{
	int oldopts = st.st_opts;

	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'j':
		st.st_mode = SM_JOBS;
		st.st_rf |= RF_CLUSTER | RF_DATASRC;
		break;
	case 'f':
		st.st_mode = SM_FAIL;
		st.st_rf |= RF_CLUSTER | RF_DATASRC;
		break;
	case 't':
		st.st_mode = SM_TEMP;
		st.st_rf |= RF_CLUSTER | RF_DATASRC;
		break;
	default:
		return;
	}
	refresh_state(oldopts);
}

void
keyh_vmode(unsigned char key, __unused int u, __unused int v)
{
	int oldopts = st.st_opts;

	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'o':
		st.st_vmode = VM_WIREDONE;
		st.st_rf |= RF_CLUSTER | RF_PERSPECTIVE | RF_GROUND | RF_SELNODE;
		break;
	case 'w':
		st.st_vmode = VM_WIRED;
		st.st_rf |= RF_CLUSTER | RF_PERSPECTIVE | RF_GROUND | RF_SELNODE;
		break;
	case 'p':
		st.st_vmode = VM_PHYSICAL;
		st.st_rf |= RF_CLUSTER | RF_PERSPECTIVE | RF_GROUND | RF_SELNODE;
		break;
	default:
		return;
	}
	refresh_state(oldopts);
}

void
keyh_option(unsigned char key, __unused int u, __unused int v)
{
	int oldopts = st.st_opts;

	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'b':
		st.st_opts ^= OP_BLEND;
		st.st_rf |= RF_CLUSTER;
		break;
	case 'D':
		st.st_opts ^= OP_DISPLAY;
		break;
	case 'd':
		st.st_opts ^= OP_CAPTURE;
		gDebugCapture = !gDebugCapture;
		if(gDebugCapture)
			begin_capture(capture_mode);
		else
			end_capture();
		break;
	case 'e':
		st.st_opts ^= OP_TWEEN;
		break;
	case 'f':
		st.st_opts ^= OP_WIVMFRAME;
		st.st_rf ^= RF_CLUSTER;
		break;
	case 'g':
		st.st_opts ^= OP_GROUND;
		break;
	case 'G': /* Ludicrous Speed */
		st.st_opts ^= OP_GOVERN;
		break;
	case 'L':
		st.st_opts ^= OP_FREELOOK;
		break;
	case 'l':
		st.st_opts ^= OP_NLABELS;
		st.st_rf |= RF_CLUSTER;
		break;
	case 'M':
		st.st_opts ^= OP_SHOWMODS;
		st.st_rf |= RF_CLUSTER;
		break;
	case 'p':
		st.st_opts ^= OP_PIPES;
		st.st_rf |= RF_CLUSTER;
		break;
	case 'P':
		st.st_opts ^= OP_SELPIPES;
		st.st_rf |= RF_CLUSTER;		/* XXX:  wrong. */
		break;
	case 't':
		st.st_opts ^= OP_TEX;
		st.st_rf |= RF_CLUSTER;
		break;
	case 's':
		st.st_opts ^= OP_DIMNONSEL;
		st.st_rf |= RF_CLUSTER;
		break;
	case 'w':
		st.st_opts ^= OP_WIREFRAME;
		st.st_rf |= RF_CLUSTER;
		break;
	default:
		return;
	}
	refresh_state(oldopts);
}

void
keyh_default(unsigned char key, __unused int u, __unused int v)
{
	int oldopts = st.st_opts;

	switch (key) {
	case 'C':
		st.st_rf |= RF_CLUSTER;
		break;
	case 'c':
		select_node(NULL);
		break;
	case 'f':
		glutKeyboardFunc(keyh_flyby);
		break;
	case 'm':
		glutKeyboardFunc(keyh_mode);
		break;
	case 'O':
		if (st.st_opts & OP_TWEEN)
			tx = ty = tz = 0;
		else {
			st.st_x = st.st_y = st.st_z = 0;
			cam_update();
		}
		break;
	case 'o':
		glutKeyboardFunc(keyh_option);
		break;
	case 'p':
		glutKeyboardFunc(keyh_panel);
		break;
	case 'q':
		exit(0);
		/* NOTREACHED */
	case 'v':
		glutKeyboardFunc(keyh_vmode);
		break;
	/* DEBUG */
	case 'z':
//		st.st_opts ^= OP_GROUND;
//		st.st_opts ^= OP_TEX;
//		st.st_opts ^= OP_BLEND;
		st.st_opts ^= OP_DEBUG;
		st.st_rf |= RF_CLUSTER;
		break;
	case '+':
	case '_': {
		float incr, *field;
		size_t j;

		incr = TRANS_INC;
		if (key == '_')
			incr *= -1;

		for (j = 0; j < njobs; j++) {
			field = &jobs[j]->j_fill.f_a;
			*field += incr;
			if (*field > 1.0f)
				*field = 1.0f;
			else if (*field < 0.0f)
				*field = 0.0f;
		}
		st.st_rf |= RF_CLUSTER;
		break;
	    }
	case '=':
	case '-': {
		float incr, *field;
		size_t j;

		incr = TRANS_INC;
		if (key == '-')
			incr *= -1;

		for (j = 0; j < NJST; j++) {
			field = &jstates[j].js_fill.f_a;
			*field += incr;
			if (*field > 1.0f)
				*field = 1.0f;
			else if (*field < 0.0f)
				*field = 0.0f;
		}
		st.st_rf |= RF_CLUSTER;
		break;
	    }
	case '[':
		st.st_winsp--;
		st.st_rf |= RF_CLUSTER | RF_GROUND | RF_PERSPECTIVE | RF_SELNODE;
		break;
	case ']':
		st.st_winsp++;
		st.st_rf |= RF_CLUSTER | RF_GROUND | RF_PERSPECTIVE | RF_SELNODE;
		break;
	default:
		return;
	}
	refresh_state(oldopts);
}

void
spkeyh_default(int key, __unused int u, __unused int v)
{
	int dir;

	dir = 0; /* gcc */
	switch (key) {
	case GLUT_KEY_LEFT:
		dir = CAMDIR_LEFT;
		break;
	case GLUT_KEY_RIGHT:
		dir = CAMDIR_RIGHT;
		break;
	case GLUT_KEY_UP:			/* Forward */
		dir = CAMDIR_FORWARD;
		break;
	case GLUT_KEY_DOWN:			/* Backward */
		dir = CAMDIR_BACK;
		break;
	case GLUT_KEY_PAGE_UP:
		dir = CAMDIR_UP;
		break;
	case GLUT_KEY_PAGE_DOWN:
		dir = CAMDIR_DOWN;
		break;
	default:
		return;
	}
	tween_pushpop(TWF_PUSH | TWF_POS);
	cam_move(dir);
	tween_pushpop(TWF_POP | TWF_POS);
}
