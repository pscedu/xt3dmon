/* $Id$ */

#include "buf.h"
#include "cam.h"
#include "cdefs.h"
#include "mon.h"

#define TRANS_INC	0.10

struct uinput uinp;

void
keyh_flyby(unsigned char key, __unused int u, __unused int v)
{
	if (key == 'F') {
		glutKeyboardFunc(keyh_default);
		glutSpecialFunc(spkeyh_default);
		active_flyby = 0;
		end_flyby();
	}
}

void
spkeyh_flyby(__unused int key, __unused int u, __unused int v) 
{
}

void
keyh_uinput(unsigned char key, __unused int u, __unused int v)
{
	switch (key) {
	case 13: /* enter */
		uinp.uinp_callback();
		/* FALLTHROUGH */
	case 27: /* escape */
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		glutKeyboardFunc(keyh_default);
		break;
	case 8:
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
			panel_toggle(1 << j);
		break;
	case 'c':
		panel_toggle(PANEL_CMD);
		glutKeyboardFunc(keyh_uinput);
		break;
	case 'g':
		panel_toggle(PANEL_GOTO);
		glutKeyboardFunc(keyh_uinput);
		break;
	case 'n':
		panel_toggle(PANEL_NINFO);
		break;
	case 'l':
		panel_toggle(PANEL_LEGEND);
		break;
	case 'f':
		panel_toggle(PANEL_FPS);
		break;
	case 'F':
		panel_toggle(PANEL_FLYBY);
		break;
	}
	restore_state(0);
}

void
keyh_mode(unsigned char key, __unused int u, __unused int v)
{
	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'j':
		st.st_mode = SM_JOBS;
		st.st_ro |= RO_COMPILE | RO_RELOAD;
		break;
	case 'f':
		st.st_mode = SM_FAIL;
		st.st_ro |= RO_COMPILE | RO_RELOAD;
		break;
	case 't':
		st.st_mode = SM_TEMP;
		st.st_ro |= RO_COMPILE | RO_RELOAD;
		break;
	}
	restore_state(0);
}

void
keyh_vmode(unsigned char key, __unused int u, __unused int v)
{
	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'l':
		st.st_vmode = VM_LOGICAL;
		st.st_ro |= RO_COMPILE;
		break;
	case 'p':
		st.st_vmode = VM_PHYSICAL;
		st.st_ro |= RO_COMPILE;
		break;
	}
	restore_state(0);
}

void
keyh_default(unsigned char key, __unused int u, __unused int v)
{
	switch (key) {
	case 'b':
		st.st_opts ^= OP_BLEND;
		st.st_ro |= RO_COMPILE;
		break;
	case 'c':
		if (st.st_selnode != NULL) {
			st.st_selnode->n_state = st.st_selnode->n_savst;
			st.st_selnode = NULL;
		}
		break;
	case 'C':
		system("rm flyby.data");
		break;
	case 'D':
		st.st_opts ^= OP_DISPLAY;
		break;
	case 'd':
		st.st_opts ^= OP_CAPTURE;
		break;
	case 'e':
		if (st.st_opts & OP_TWEEN)
			st.st_opts &= ~OP_TWEEN;
		else
			st.st_opts |= OP_TWEEN;
		break;
	case 'f':
		build_flyby = !build_flyby;
		if(build_flyby)
			begin_flyby('w');
		else if(!build_flyby && !active_flyby)
			end_flyby();
		break;
	case 'F':
		active_flyby = 1;
		begin_flyby('r');
		glutKeyboardFunc(keyh_flyby);
		glutSpecialFunc(spkeyh_flyby);
		break;
	case 'g':
		st.st_opts ^= OP_GROUND;
		break;
	case 'm':
		glutKeyboardFunc(keyh_mode);
		break;
	case 'p':
		glutKeyboardFunc(keyh_panel);
		break;
	case 'P':
		printf("pos[%.2f,%.2f,%.2f] look[%.2f,%.2f,%.2f]\n",
		    st.st_x, st.st_y, st.st_z,
		    st.st_lx, st.st_ly, st.st_lz);
		break;
	case 'q':
		exit(0);
		/* NOTREACHED */
	case 'T':
		st.st_alpha_fmt = (st.st_alpha_fmt == GL_RGBA ? GL_INTENSITY : GL_RGBA);
		st.st_ro |= RO_TEX | RO_COMPILE;
		break;
	case 't':
		st.st_opts ^= OP_TEX;
		st.st_ro |= RO_COMPILE;
		st.st_ro |= RO_TEXTURE;
		break;
	case 'v':
		glutKeyboardFunc(keyh_vmode);
		break;
	case 'w':
		st.st_opts ^= OP_WIRES;
		st.st_ro |= RO_COMPILE;
		break;
	case '+':
		st.st_alpha_job += (st.st_alpha_job + TRANS_INC > 1.0 ? 0.0 : TRANS_INC);
		st.st_ro |= RO_COMPILE;
		break;
	case '_':
		st.st_alpha_job -= (st.st_alpha_job + TRANS_INC < 0.0 ? 0.0 : TRANS_INC);
		st.st_ro |= RO_COMPILE;
		break;
	case '=':
		st.st_alpha_oth += (st.st_alpha_oth + TRANS_INC > 1.0 ? 0.0 : TRANS_INC);
		st.st_ro |= RO_COMPILE;
		break;
	case '-':
		st.st_alpha_oth -= (st.st_alpha_oth + TRANS_INC < 0.0 ? 0.0 : TRANS_INC);
		st.st_ro |= RO_COMPILE;
		break;
	}
	restore_state(0);
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
	move_cam(dir);
}
