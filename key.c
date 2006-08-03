/* $Id$ */

#include "mon.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "buf.h"
#include "cam.h"
#include "deusex.h"
#include "env.h"
#include "flyby.h"
#include "gl.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "pathnames.h"
#include "route.h"
#include "select.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"
#include "uinp.h"
#include "xmath.h"

struct uinput	uinp;
int		keyh = KEYH_DEF;

void
gl_spkeyh_null(__unused int key, __unused int u, __unused int v)
{
	flyby_rstautoto();
}

void
gl_spkeyh_actflyby(__unused int key, __unused int u, __unused int v)
{
	flyby_end();
}

void
gl_keyh_actflyby(__unused unsigned char key, __unused int u, __unused int v)
{
	if (key == ' ') {
		st.st_opts ^= OP_STOP;
		return;
	}
	flyby_end();
}

void
gl_keyh_flyby(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'c':
		flyby_clear();
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
		opt_flip(OP_LOOPFLYBY);
		break;
	}
}

void
gl_keyh_uinput(unsigned char key, __unused int u, __unused int v)
{
	struct panel *p;
	int opts;

	flyby_rstautoto();

	p = uinp.uinp_panel;
	uinp.uinp_opts |= UINPO_DIRTY;
	switch (key) {
	case 13:	/* enter */
		glutKeyboardFunc(gl_keyh_default);
		uinp.uinp_panel = NULL;

		opts = uinp.uinp_opts;
		uinp.uinp_callback();
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		if ((opts & UINPO_LINGER) == 0)
			panel_tremove(p);
		break;
	case 27:	/* escape */
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		glutKeyboardFunc(gl_keyh_default);
		uinp.uinp_panel = NULL;
		panel_tremove(p);
		break;
	case 127:	/* MacOS delete */
	case 8:		/* backspace */
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
gl_keyh_panel(unsigned char key, __unused int u, __unused int v)
{
	int j;

	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'a':
		for (j = 0; j < NPANELS; j++)
			if ((pinfo[j].pi_opts & PF_UINP) == 0)
				panel_toggle(1 << j);
		break;
	case 'c':
		panel_toggle(PANEL_CMD);
		break;
	case 'D':
		panel_toggle(PANEL_DMODE);
		break;
	case 'd':
		panel_toggle(PANEL_DATE);
		break;
	case 'F':
		panel_toggle(PANEL_FLYBY);
		break;
	case 'f':
		panel_toggle(PANEL_FPS);
		break;
	case 'g':
		panel_toggle(PANEL_GOTONODE);
		break;
	case 'j':
		panel_toggle(PANEL_GOTOJOB);
		break;
	case 'L':
		panel_toggle(PANEL_LOGIN);
		break;
	case 'l':
		panel_toggle(PANEL_LEGEND);
		break;
	case 'n':
		panel_toggle(PANEL_NINFO);
		break;
	case 'o':
		panel_toggle(PANEL_OPTS);
		break;
	case 'P':
		panel_toggle(PANEL_PANELS);
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
	case 'V':
		panel_toggle(PANEL_VMODE);
		break;
	case '~':
		panel_toggle(PANEL_EGGS);
		break;
	}
}

void
gl_keyh_dmode(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'j':
		st.st_dmode = DM_JOB;
		break;
	case 'f':
		st.st_dmode = DM_FAIL;
		break;
	case 's':
		st.st_dmode = DM_SAME;
		break;
	case 't':
		st.st_dmode = DM_TEMP;
		break;
	case 'y':
		st.st_dmode = DM_YOD;
		break;
	case 'r':
		st.st_dmode = DM_RTUNK;
		break;
	default:
		return;
	}
	st.st_rf |= RF_DMODE;
}

void
gl_keyh_vmode(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'o':
		st.st_vmode = VM_WIONE;
		break;
	case 'w':
		st.st_vmode = VM_WIRED;
		break;
	case 'p':
		st.st_vmode = VM_PHYS;
		break;
	default:
		return;
	}
	st.st_rf |= RF_VMODE;
}

void
gl_keyh_option(unsigned char key, __unused int u, __unused int v)
{
	int opts = 0;

	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'D':
		opts |= OP_DISPLAY;
		break;
	case 'd':
		opts |= OP_CAPTURE;
		break;
	case 'e':
		opts |= OP_TWEEN;
		break;
	case 'G':
		opts |= OP_GOVERN;
		break;
	case 'g':
		opts |= OP_GROUND;
		break;
	case 'l':
		opts |= OP_NLABELS;
		break;
	case 'M':
		opts |= OP_MODSKELS;
		break;
	case 'n':
		opts |= OP_NODEANIM;
		break;
	case 'P':
		opts |= OP_SELPIPES;
		break;
	case 'p':
		opts |= OP_PIPES;
		break;
	case 's':
		opts |= OP_SKEL;
		break;
	case 't':
		opts |= OP_TEX;
		break;
	case 'w':
		opts |= OP_FRAMES;
		break;
	case 'x':
		opts |= OP_DEUSEX;
		break;
	default:
		return;
	}
	opt_flip(opts);
}

void
gl_keyh_decr(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
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
	st.st_rf |= RF_CLUSTER | RF_GROUND | RF_CAM | RF_SELNODE | RF_NODESWIV;
}

void
gl_keyh_incr(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
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
	st.st_rf |= RF_CLUSTER | RF_GROUND | RF_CAM | RF_SELNODE | RF_NODESWIV;
}

void
gl_spkeyh_wiadj(int key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	spkey = glutGetModifiers();
	switch (key) {
	case GLUT_KEY_PAGE_UP:
		st.st_wioff.iv_y++;
		break;
	case GLUT_KEY_PAGE_DOWN:
		st.st_wioff.iv_y--;
		break;
	case GLUT_KEY_UP:
		st.st_wioff.iv_x++;
		break;
	case GLUT_KEY_DOWN:
		st.st_wioff.iv_x--;
		break;
	case GLUT_KEY_LEFT:
		st.st_wioff.iv_z--;
		break;
	case GLUT_KEY_RIGHT:
		st.st_wioff.iv_z++;
		break;
	default:
		return;
	}
	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_NODESWIV;
}

void
gl_spkeyh_node(int key, __unused int u, __unused int v)
{
	struct selnode *sn;
	struct node *n;
	int rd;

	flyby_rstautoto();
	spkey = glutGetModifiers();
	switch (key) {
	case GLUT_KEY_PAGE_UP:
		rd = RD_POSY;
		break;
	case GLUT_KEY_PAGE_DOWN:
		rd = RD_NEGY;
		break;
	case GLUT_KEY_UP:
		rd = RD_POSX;
		break;
	case GLUT_KEY_DOWN:
		rd = RD_NEGX;
		break;
	case GLUT_KEY_LEFT:
		rd = RD_NEGZ;
		break;
	case GLUT_KEY_RIGHT:
		rd = RD_POSZ;
		break;
	default:
		return;
	}
	if (!SLIST_EMPTY(&selnodes))
		SLIST_FOREACH(sn, &selnodes, sn_next) {
			n = node_neighbor(st.st_vmode, sn->sn_nodep, rd, NULL);
			if (spkey & GLUT_ACTIVE_SHIFT)
				sn_add(n, &fv_zero);
			else
				sn_replace(sn, n);
		}
}

void
gl_keyh_keyh(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'n':
		glutSpecialFunc(gl_spkeyh_node);
		keyh = KEYH_NEIGH;
		break;
	case 'w':
		glutSpecialFunc(gl_spkeyh_wiadj);
		keyh = KEYH_WIADJ;
		break;
	default:
		glutSpecialFunc(gl_spkeyh_default);
		keyh = KEYH_DEF;
		break;
	}
}

void
gl_keyh_alpha(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'j':
		nc_runall(fill_setxparent);
		nc_runsn(fill_setopaque);
		break;
	case 'r':
		nc_runall(fill_setopaque);
		break;
	case 's':
		nc_runall(fill_setxparent);
		break;
	default:
		return;
	}
}

void
gl_keyh_wioffdecr(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'x':
		st.st_wioff.iv_x--;
		break;
	case 'y':
		st.st_wioff.iv_y--;
		break;
	case 'z':
		st.st_wioff.iv_z--;
		break;
	case '0':
		st.st_wioff.iv_x = 0;
		st.st_wioff.iv_y = 0;
		st.st_wioff.iv_z = 0;
		break;
	case '{':
		st.st_wioff.iv_x--;
		st.st_wioff.iv_y--;
		st.st_wioff.iv_z--;
		break;
	}
	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_NODESWIV;
}

void
gl_keyh_wioffincr(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'x':
		st.st_wioff.iv_x++;
		break;
	case 'y':
		st.st_wioff.iv_y++;
		break;
	case 'z':
		st.st_wioff.iv_z++;
		break;
	case '0':
		st.st_wioff.iv_x = 0;
		st.st_wioff.iv_y = 0;
		st.st_wioff.iv_z = 0;
		break;
	case '}':
		st.st_wioff.iv_x++;
		st.st_wioff.iv_y++;
		st.st_wioff.iv_z++;
		break;
	}
	st.st_rf |= RF_CLUSTER | RF_SELNODE | RF_GROUND | RF_NODESWIV;
}

void
gl_keyh_pipes(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case 'r': /* color by route errors */
		st.st_pipemode = PM_RTE;
		break;
	case 'd': /* color by direction */
		st.st_pipemode = PM_DIR;
		break;
	}
	st.st_rf |= RF_CLUSTER | RF_SELNODE;
}

void
gl_keyh_route(unsigned char key, __unused int u, __unused int v)
{
	flyby_rstautoto();
	glutKeyboardFunc(gl_keyh_default);
	switch (key) {
	case '+':
		st.st_rtepset = RPS_POS;
		break;
	case '-':
		st.st_rtepset = RPS_NEG;
		break;
	case 'r': /* inc */
		st.st_rtepset = (st.st_rtepset + 1) % NRPS;
		break;
	case 'R': /* recover */
		st.st_rtetype = RT_RECOVER;
		break;
	case 'F': /* fatal */
		st.st_rtetype = RT_FATAL;
		break;
	case 'T': /* router */
		st.st_rtetype = RT_ROUTER;
		break;
	}
	st.st_rf |= RF_CLUSTER | RF_DMODE;
}

void
gl_keyh_server(unsigned char key, __unused int u, __unused int v)
{
	switch (key) {
	case 'q':
		exit(0);
		/* NOTREACHED */
	}
}

void
gl_keyh_default(unsigned char key, int u, int v)
{
	flyby_rstautoto();
	switch (key) {
	case 1: /* ^A */
		sn_addallvis();
		break;
	case '2': case '4':
	case '6': case '8': {
		double du, dv;

		du = dv = 0.0;
		switch (key) {
		case '2': dv = -0.02; break;
		case '4': du =  0.02; break;
		case '6': du = -0.02; break;
		case '8': dv =  0.02; break;
		}

		tween_push(TWF_LOOK | TWF_POS | TWF_UP);
		cam_revolvefocus(du, -dv);
		tween_pop(TWF_LOOK | TWF_POS | TWF_UP);
		break;
	    }
	case 'a':
		glutKeyboardFunc(gl_keyh_alpha);
		break;
	case 'C':
		st.st_rf |= RF_CLUSTER | RF_WIREP | RF_SELNODE;
		break;
	case 'c':
		sn_clear();
		break;
	case 'F': {
		int t;

		t = window_ids[WINID_LEFT];
		window_ids[WINID_LEFT] = window_ids[WINID_RIGHT];
		window_ids[WINID_RIGHT] = t;
		break;
	    }
	case 'f':
		glutKeyboardFunc(gl_keyh_flyby);
		break;
	case 'I': {
		struct selnode *sn;

		SLIST_FOREACH(sn, &selnodes, sn_next)
			sn->sn_nodep->n_flags ^= NF_SHOW;
		break;
	}
	case 'i': {
		struct selnode *sn;

		SLIST_FOREACH(sn, &selnodes, sn_next)
			printf("%d%s", sn->sn_nodep->n_nid,
			    SLIST_NEXT(sn, sn_next) ? "," : "\n");
		break;
	    }
	case 'k':
		glutKeyboardFunc(gl_keyh_keyh);
		break;
	case 'm':
		glutKeyboardFunc(gl_keyh_dmode);
		break;
	case 'O':
		tween_push(TWF_LOOK | TWF_POS | TWF_UP);
		cam_bird();
		tween_pop(TWF_LOOK | TWF_POS | TWF_UP);
		break;
	case 'o':
		glutKeyboardFunc(gl_keyh_option);
		break;
	case 'P':
		glutKeyboardFunc(gl_keyh_pipes);
		break;
	case 'p':
		glutKeyboardFunc(gl_keyh_panel);
		break;
	case 'q':
		exit(0);
		/* NOTREACHED */
	case 'R':
		st.st_rf |= RF_DATASRC;
		break;
	case 'r':
		glutKeyboardFunc(gl_keyh_route);
		break;
	case 'v':
		glutKeyboardFunc(gl_keyh_vmode);
		break;
	case '{':
		glutKeyboardFunc(gl_keyh_wioffdecr);
		break;
	case '}':
		glutKeyboardFunc(gl_keyh_wioffincr);
		break;
	case '+': case '-':
	case '=': case '_':
		mousev.iv_u = u;
		mousev.iv_v = v;
		gl_select(key == '+' || key == '=' ?
		    SPF_SQUIRE : SPF_DESQUIRE);
		break;
	case '[':
		glutKeyboardFunc(gl_keyh_decr);
		break;
	case ']':
		glutKeyboardFunc(gl_keyh_incr);
		break;
	case ' ':
		if (st.st_opts & OP_DEUSEX) {
			opt_flip(OP_STOP);
			st.st_opts ^= OP_TWEEN;
		}
		break;
	default:
		return;
	}
}

void
gl_spkeyh_default(int key, __unused int u, __unused int v)
{
	float r, adj, amt;
	int dir;

	flyby_rstautoto();

	amt = 0.0f; /* gcc */
	dir = 0; /* gcc */
	switch (key) {
	case GLUT_KEY_F12:
		restart();
		/* NOTREACHED */
	case GLUT_KEY_HOME:
		tween_push(TWF_UP);
		cam_roll(0.1);
		tween_pop(TWF_UP);
		return;
	case GLUT_KEY_END:
		tween_push(TWF_UP);
		cam_roll(-0.1);
		tween_pop(TWF_UP);
		return;
	case GLUT_KEY_LEFT:
		dir = DIR_LEFT;
		break;
	case GLUT_KEY_RIGHT:
		dir = DIR_RIGHT;
		break;
	case GLUT_KEY_UP:
		dir = DIR_FORW;
		break;
	case GLUT_KEY_DOWN:
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
	case VM_PHYS:
		r = sqrt(SQUARE(st.st_x - XCENTER) + SQUARE(st.st_z - ZCENTER));
		adj = pow(2, r / (ROWWIDTH / 2.0f));
		if (adj > 50.0f)
			adj = 50.0f;
		amt *= adj;
		break;
	case VM_WIRED:
	case VM_WIONE:
		amt *= pow(fabs(st.st_winsp.iv_x) * fabs(st.st_winsp.iv_y) *
		    fabs(st.st_winsp.iv_z), 1/3.0);
		break;
	}

	tween_push(TWF_POS);
	cam_move(dir, amt);
	tween_pop(TWF_POS);
}
