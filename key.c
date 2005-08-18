/* $Id$ */

#include <errno.h>
#include <math.h>

#include "buf.h"
#include "cdefs.h"
#include "mon.h"

#define TRANS_INC	0.10

struct uinput uinp;
struct fvec stopv, stoplv;

void
spkeyh_null(__unused int key, __unused int u, __unused int v)
{
}

void
spkeyh_actflyby(__unused int key, __unused int u, __unused int v)
{
	flyby_end();
}

void
keyh_actflyby(__unused unsigned char key, __unused int u, __unused int v)
{
	if (key == ' ') {
		st.st_opts ^= OP_STOP;
		if (st.st_opts & OP_STOP) {
			stopv.fv_x = tv.fv_x;  stoplv.fv_x = tlv.fv_x;
			stopv.fv_y = tv.fv_y;  stoplv.fv_y = tlv.fv_y;
			stopv.fv_z = tv.fv_z;  stoplv.fv_z = tlv.fv_z;

			tv.fv_x = st.st_x;  tlv.fv_x = st.st_lx;
			tv.fv_y = st.st_y;  tlv.fv_y = st.st_ly;
			tv.fv_z = st.st_z;  tlv.fv_z = st.st_lz;
		} else {
			tv.fv_x = stopv.fv_x;  tlv.fv_x = stoplv.fv_x;
			tv.fv_y = stopv.fv_y;  tlv.fv_y = stoplv.fv_y;
			tv.fv_z = stopv.fv_z;  tlv.fv_z = stoplv.fv_z;
		}
		return;
	}
	flyby_end();
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
		switch (flyby_mode) {
		case FBM_REC:
			flyby_end();
			break;
		case FBM_PLAY:
			break;
		default:
			flyby_begin(FBM_REC);
			break;
		}
		break;
	case 'p':
		switch (flyby_mode) {
		case FBM_PLAY:
			flyby_end();
			break;
		case FBM_REC:
			break;
		default:
			flyby_begin(FBM_PLAY);
			break;
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
			panel_tremove(uinp.uinp_panel);
		break;
	case 27: /* escape */
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		glutKeyboardFunc(keyh_default);
		panel_tremove(uinp.uinp_panel);
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
	case 'm':
		panel_toggle(PANEL_MEM);
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
	case 'S':
		panel_toggle(PANEL_STATUS);
		break;
	case '~':
		panel_toggle(PANEL_EGGS);
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
		break;
	case 'w':
		st.st_vmode = VM_WIRED;
		break;
	case 'p':
		st.st_vmode = VM_PHYSICAL;
		break;
	default:
		return;
	}
	st.st_rf |= RF_CLUSTER | RF_CAM | RF_GROUND | RF_SELNODE | RF_DATASRC;
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
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
		break;
	case 'D':
		st.st_opts ^= OP_DISPLAY;
		break;
	case 'd':
		st.st_opts ^= OP_CAPTURE;
		if (st.st_opts & OP_CAPTURE)
			capture_begin(capture_mode);
		else
			capture_end();
		break;
	case 'e':
		st.st_opts ^= OP_TWEEN;
		break;
	case 'f':
		st.st_opts ^= OP_WIVMFRAME;
		st.st_rf ^= RF_CLUSTER | RF_SELNODE;
		break;
	case 'G': /* Ludicrous Speed */
		st.st_opts ^= OP_GOVERN;
		break;
	case 'g':
		st.st_opts ^= OP_GROUND;
		break;
	case 'l':
		st.st_opts ^= OP_NLABELS;
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
		break;
	case 'M':
		st.st_opts ^= OP_SHOWMODS;
		st.st_rf |= RF_CLUSTER;
		break;
	case 'P':
		st.st_opts ^= OP_SELPIPES;
		st.st_rf |= RF_CLUSTER | RF_SELNODE;		/* XXX:  wrong. */
		break;
	case 'p':
		st.st_opts ^= OP_PIPES;
		st.st_rf |= RF_CLUSTER;
		break;
	case 't':
		st.st_opts ^= OP_TEX;
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
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
keyh_decr(unsigned char key, __unused int u, __unused int v)
{
	int oldopts = st.st_opts;

	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'x':
		st.st_winsp.iv_x--;
		break;
	case 'y':
		st.st_winsp.iv_y--;
		break;
	case 'z':
		st.st_winsp.iv_z--;
		break;
	case '[':
		st.st_winsp.iv_x--;
		st.st_winsp.iv_y--;
		st.st_winsp.iv_z--;
		break;
	default:
		return;
	}
	st.st_rf |= RF_CLUSTER | RF_GROUND | RF_CAM | RF_SELNODE;
	refresh_state(oldopts);
}

void
keyh_incr(unsigned char key, __unused int u, __unused int v)
{
	int oldopts = st.st_opts;

	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'x':
		st.st_winsp.iv_x++;
		break;
	case 'y':
		st.st_winsp.iv_y++;
		break;
	case 'z':
		st.st_winsp.iv_z++;
		break;
	case ']':
		st.st_winsp.iv_x++;
		st.st_winsp.iv_y++;
		st.st_winsp.iv_z++;
		break;
	default:
		return;
	}
	st.st_rf |= RF_CLUSTER | RF_GROUND | RF_CAM | RF_SELNODE;
	refresh_state(oldopts);
}

void
spkeyh_node(int key, __unused int u, __unused int v)
{
	struct selnode *sn;
	struct node *n;
	int dir;

	spkey = glutGetModifiers();
	switch (key) {
	case GLUT_KEY_PAGE_UP:
		dir = DIR_UP;
		break;
	case GLUT_KEY_PAGE_DOWN:
		dir = DIR_DOWN;
		break;
	case GLUT_KEY_UP:
		dir = DIR_FORWARD;
		break;
	case GLUT_KEY_DOWN:
		dir = DIR_BACK;
		break;
	case GLUT_KEY_LEFT:
		dir = DIR_LEFT;
		break;
	case GLUT_KEY_RIGHT:
		dir = DIR_RIGHT;
		break;
	}
	if (!SLIST_EMPTY(&selnodes))
		SLIST_FOREACH(sn, &selnodes, sn_next) {
			n = node_neighbor(sn->sn_nodep, 1, dir);
			if (spkey & GLUT_ACTIVE_SHIFT)
				sn_add(n);
			else
				sn_replace(sn, n);
		}
}

void
keyh_keyh(unsigned char key, __unused int u, __unused int v)
{
	glutKeyboardFunc(keyh_default);
	switch (key) {
	case 'n':
		glutSpecialFunc(spkeyh_node);
		break;
	default:
		glutSpecialFunc(spkeyh_default);
		break;
	}
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
		sn_clear();
		break;
	case 'f':
		glutKeyboardFunc(keyh_flyby);
		break;
	case 'k':
		glutKeyboardFunc(keyh_keyh);
		break;
	case 'm':
		glutKeyboardFunc(keyh_mode);
		break;
	case 'O': {
		struct fvec v;

		v.fv_x = v.fv_y = v.fv_z = 0.0f;
		tween_push(TWF_LOOK | TWF_POS);
		cam_goto(&v);
		st.st_lx = 1.0f;
		st.st_ly = 0.0f;
		st.st_lz = 0.0f;
		tween_pop(TWF_LOOK | TWF_POS);
		break;
	    }
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
//		gShowMods = !gShowMods;
		break;
	case '+':
	case '_': {
		float incr, *field;
		size_t j;

		incr = TRANS_INC;
		if (key == '_')
			incr *= -1;

		for (j = 0; j < job_list.ol_cur; j++) {
			field = &job_list.ol_jobs[j]->j_fill.f_a;
			*field += incr;
			if (*field > 1.0f)
				*field = 1.0f;
			else if (*field < 0.0f)
				*field = 0.0f;
		}
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
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
		st.st_rf |= RF_CLUSTER | RF_SELNODE;
		break;
	    }
	case '[':
		glutKeyboardFunc(keyh_decr);
		break;
	case ']':
		glutKeyboardFunc(keyh_incr);
		break;
	default:
		return;
	}
	refresh_state(oldopts);
}

void
spkeyh_default(int key, __unused int u, __unused int v)
{
	float r, adj, amt;
	int dir;

	amt = 0.0f; /* gcc */
	dir = 0; /* gcc */
	switch (key) {
	case GLUT_KEY_HOME:
		tween_push(TWF_ROLL);
		cam_roll(0.1f);
		tween_pop(TWF_ROLL);
		return;
	case GLUT_KEY_END:
		tween_push(TWF_ROLL);
		cam_roll(-0.1f);
		tween_pop(TWF_ROLL);
		return;
	case GLUT_KEY_LEFT:
		dir = DIR_LEFT;
		break;
	case GLUT_KEY_RIGHT:
		dir = DIR_RIGHT;
		break;
	case GLUT_KEY_UP:			/* Forward */
		dir = DIR_FORWARD;
		break;
	case GLUT_KEY_DOWN:			/* Backward */
		dir = DIR_BACK;
		break;
	case GLUT_KEY_PAGE_UP:
		dir = DIR_UP;
		break;
	case GLUT_KEY_PAGE_DOWN:
		dir = DIR_DOWN;
		break;
	default:
		return;
	}

	amt = 0.3f;
	switch (st.st_vmode) {
	case VM_PHYSICAL:
		r = sqrt(SQUARE(st.st_x - XCENTER) + SQUARE(st.st_z - ZCENTER));
		adj = pow(2, r / (ROWWIDTH / 2.0f));
		if (adj > 50.0f)
			adj = 50.0f;
		amt *= adj;
		break;
	case VM_WIRED:
	case VM_WIREDONE:
		amt *= cbrt(st.st_winsp.iv_x * st.st_winsp.iv_y *
		    st.st_winsp.iv_z);
		break;
	}

	tween_push(TWF_POS);
	cam_move(dir, amt);
	tween_pop(TWF_POS);
}
