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

/*
 * General notes for the panel-handling code.
 *
 *	(*) Panels are redrawn when the POPT_DIRTY flag is set in their
 *	    structure.  This means only that GL needs to re-render
 *	    the panel.  Panels must determine on their on, in their
 *	    refresh function, when their information itself is
 *	    "dirty."
 *
 * Panels have the following drawn structure:
 *
 *      <--------------------- p_w --------------------->
 *	+-----------------------------------------------+ / \
 *	|						|  |
 *	|   - - - - - - - - - - - - - - - - - - - - -	|  |
 *	|  |p_str		|		     |	|  |
 *	|   PANEL_PADDING				|  |
 *	|  |+---+		|+---+		     |	|  |
 *	|   |   | pw_str	 |   | pw_str		|
 *	|  |+---+		|+---+		     |	|  p_h
 *	|   PANEL_PADDING				|
 *	|  |+---+		|+---+		     |	|  |
 *	|   |   | pw_str	 |   | pw_str		|  |
 *	|  |+---+		|+---+		     |	|  |
 *	|   - - - - - - - - - - - - - - - - - - - - -	|  |
 *	|						|  |
 *	+-----------------------------------------------+ \ /
 *
 */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "draw.h"
#include "env.h"
#include "fill.h"
#include "flyby.h"
#include "gl.h"
#include "objlist.h"
#include "panel.h"
#include "queue.h"
#include "select.h"
#include "uinp.h"
#include "util.h"

#define LETTER_WIDTH	8
#define LETTER_HEIGHT	13
#define PANEL_PADDING	3
#define PANEL_BWIDTH	1
#define PWIDGET_LENGTH	16
#define PWIDGET_HEIGHT	LETTER_HEIGHT
#define PWLABEL_MAXLEN	((winv.iv_w - 1) / 2 / 2 - PWIDGET_PADDING)
#define PWIDGET_PADDING	2
#define PFONT		GLUT_BITMAP_8_BY_13

void uinpcb_ss(void);
void uinpcb_eggs(void);

struct pinfo pinfo[] = {
 /*  0 */ { "fps",	"FPS",			panel_refresh_fps,	PSTICK_TR, 0,					0,		NULL },
 /*  1 */ { "ninfo",	"Node Info",		panel_refresh_ninfo,	PSTICK_TR, 0,					0,		NULL },
 /*  2 */ { "cmd",	"Command",		panel_refresh_cmd,	PSTICK_TR, PIF_UINP | PIF_FBIGN | PIF_HIDE,	UINPO_LINGER,	uinpcb_cmd },
 /*  3 */ { "legend",	"Legend",		panel_refresh_legend,	PSTICK_TR, 0,					0,		NULL },
 /*  4 */ { "fbstat",	"Flyby Controls",	panel_refresh_flyby,	PSTICK_TR, PIF_FBIGN,				0,		NULL },
 /*  5 */ { "gotonode",	"Goto Node",		panel_refresh_gotonode,	PSTICK_TR, PIF_UINP | PIF_FBIGN,		0,		uinpcb_gotonode },
 /*  6 */ { "cam",	"Camera Position",	panel_refresh_pos,	PSTICK_TR, 0,					0,		NULL },
 /*  7 */ { "sshot",	"Screenshot",		panel_refresh_ss,	PSTICK_TR, PIF_UINP | PIF_FBIGN,		0,		uinpcb_ss },
 /*  8 */ { "status",	"Status Log",		panel_refresh_status,	PSTICK_TR, 0,					0,		NULL },
 /*  9 */ { "eggs",	NULL,			panel_refresh_eggs,	PSTICK_TR, PIF_UINP | PIF_FBIGN | PIF_HIDE,	0,		uinpcb_eggs },
 /* 10 */ { "date",	"Date",			panel_refresh_date,	PSTICK_BL, PIF_XPARENT,				0,		NULL },
 /* 11 */ { "options",	"Options",		panel_refresh_opts,	PSTICK_TL, PIF_FBIGN,				0,		NULL },
 /* 12 */ { "gotojob",	"Goto Job",		panel_refresh_gotojob,	PSTICK_TR, PIF_UINP | PIF_FBIGN,		0,		uinpcb_gotojob },
 /* 13 */ { "panels",	NULL,			panel_refresh_panels,	PSTICK_TL, PIF_HIDE | PIF_FBIGN,		0,		NULL },
 /* 14 */ { "login",	"Login",		panel_refresh_login,	PSTICK_TR, PIF_UINP | PIF_FBIGN,		UINPO_LINGER,	uinpcb_login },
 /* 15 */ { "help",	"Help",			panel_refresh_help,	PSTICK_BR, PIF_HIDE | PIF_FBIGN | PIF_XPARENT,	0,		NULL },
 /* 16 */ { "vmode",	"View Mode",		panel_refresh_vmode,	PSTICK_TL, 0,					0,		NULL },
 /* 17 */ { "dmode",	"Data Mode",		panel_refresh_dmode,	PSTICK_TL, 0,					0,		NULL },
 /* 18 */ { "reel",	"Reel",			panel_refresh_reel,	PSTICK_TR, PIF_FBIGN,				0,		NULL },
 /* 19 */ { "pipe",	"Pipe Mode",		panel_refresh_pipe,	PSTICK_TR, 0,					0,		NULL },
 /* 20 */ { "sstar",	"Seastar",		panel_refresh_sstar,	PSTICK_TR, 0,					0,		NULL },
 /* 21 */ { "wiadj",	"Wired Controls",	panel_refresh_wiadj,	PSTICK_BL, 0,					0,		NULL },
 /* 22 */ { "rt",	"Route Controls",	panel_refresh_rt,	PSTICK_TR, 0,					0,		NULL },
 /* 23 */ { "fbcho",	"Flyby Chooser",	panel_refresh_fbcho,	PSTICK_TR, PIF_FBIGN,				0,		NULL },
 /* 24 */ { "fbcreat",	"Flyby Creator",	panel_refresh_fbnew,	PSTICK_TR, PIF_FBIGN | PIF_UINP,		0,		uinpcb_fbnew },
 /* 25 */ { "compass",	"Compass",		panel_refresh_cmp,	PSTICK_BL, PIF_FBIGN,				0,		NULL },
 /* 26 */ { "keyh",	"Key Handler",		panel_refresh_keyh,	PSTICK_BL, 0,					0,		NULL },
 /* 27 */ { "dxcho",	"Deus Ex Chooser",	panel_refresh_dxcho,	PSTICK_TR, PIF_FBIGN,				0,		NULL },
 /* 28 */ { "dscho",	"Dataset Chooser",	panel_refresh_dscho,	PSTICK_TR, PIF_FBIGN,				0,		NULL },
 /* 29 */ { "vnmode",	"VNeighbor Mode",	panel_refresh_vnmode,	PSTICK_TL, 0,					0,		NULL },
 /* 30 */ { "pipedim",	"Pipe Dimensions",	panel_refresh_pipedim,	PSTICK_TR, 0,					0,		NULL }
};

#define PVOFF_TL 0
#define PVOFF_TR 1
#define PVOFF_BL 2
#define PVOFF_BR 3
#define NPVOFF 4

struct panels	 panels = TAILQ_HEAD_INITIALIZER(panels);
int		 panel_offset[NPVOFF];
int		 exthelp;

void
panel_free(struct panel *p)
{
	struct pwidget *pw, *nextp;

	if (p->p_dl[WINID_LEFT])
		glDeleteLists(p->p_dl[WINID_LEFT], 1);
	if (p->p_dl[WINID_RIGHT])
		glDeleteLists(p->p_dl[WINID_RIGHT], 1);
	TAILQ_REMOVE(&panels, p, p_link);
	for (pw = SLIST_FIRST(&p->p_widgets);
	    pw != SLIST_END(&p->p_widgets);
	    pw = nextp) {
		nextp = SLIST_NEXT(pw, pw_next);
		free(pw);
	}
	free(p->p_str);
	free(p->p_wcw);
	pwidget_grouplist_free(p);
	free(p);
}

void
draw_shadow_panels(void)
{
	unsigned int name;
	struct pwidget *pw;
	struct glname *gn;
	struct panel *p;

	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, winv.iv_w, 0.0, winv.iv_h);

	TAILQ_FOREACH(p, &panels, p_link) {
		SLIST_FOREACH(pw, &p->p_widgets, pw_next) {
			pw->pw_gn = NULL;
			if (pw->pw_cb == NULL || pw->pw_flags & PWF_HIDE)
				continue;
			name = gsn_get(GNF_2D | GNF_PWIDGET,
			    &fv_zero, pw->pw_cb,
			    pw->pw_arg_int, pw->pw_arg_int2,
			    pw->pw_arg_ptr, pw->pw_arg_ptr2);
			gn = obj_get(&name, &glname_list);
			gn->gn_u = pw->pw_u;
			gn->gn_v = pw->pw_v;
			gn->gn_h = pw->pw_h;
			gn->gn_w = pw->pw_w;
			gn->gn_arg_ptr3 = pw;
			pw->pw_gn = gn;

			glPushMatrix();
			glPushName(name);
			glBegin(GL_QUADS);
			glVertex2d(pw->pw_u,		pw->pw_v);
			glVertex2d(pw->pw_u + pw->pw_w,	pw->pw_v);
			glVertex2d(pw->pw_u + pw->pw_w,	pw->pw_v - pw->pw_h + 1);
			glVertex2d(pw->pw_u,		pw->pw_v - pw->pw_h + 1);
			glEnd();
			glPopName();
			glPopMatrix();
		}

		name = gsn_get(GNF_2D, &fv_zero, gscb_panel,
		    p->p_id, 0, NULL, NULL);
		gn = obj_get(&name, &glname_list);
		gn->gn_u = p->p_u;
		gn->gn_v = p->p_v;
		gn->gn_h = p->p_h;
		gn->gn_w = p->p_w;

		glPushMatrix();
		glPushName(name);
		glBegin(GL_QUADS);
		glVertex2d(p->p_u,		p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h + 1);
		glVertex2d(p->p_u,		p->p_v - p->p_h + 1);
		glEnd();
		glPopName();
		glPopMatrix();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

__inline void
draw_pwbg(struct fill *fp, int uoff, int voff)
{
	int max, blend;
	float tl, th;

	if (fp->f_flags & FF_TEX) {
		blend = (fp->f_a != 1.0 && (fp->f_flags & FF_OPAQUE) == 0);

		if (blend) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, blend ?
		    fp->f_texid_a[wid] : fp->f_texid[wid]);

		/* -2 to account for border. */
		max = MAX(PWIDGET_LENGTH - 2, PWIDGET_HEIGHT - 2);
		tl = TEXCOORD(PWIDGET_LENGTH - 2, max);
		th = TEXCOORD(PWIDGET_HEIGHT - 2, max);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
		    blend ? GL_BLEND : GL_REPLACE);

		if (blend)
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,
			    fp->f_rgba);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 3.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);
		glVertex2d(uoff + 1,			voff);
		glTexCoord2d(tl, 0.0);
		glVertex2d(uoff + PWIDGET_LENGTH - 1,	voff);
		glTexCoord2d(tl, th);
		glVertex2d(uoff + PWIDGET_LENGTH - 1,	voff - PWIDGET_HEIGHT + 2);
		glTexCoord2d(0.0, th);
		glVertex2d(uoff + 1,			voff - PWIDGET_HEIGHT + 2);
		glEnd();

		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_POLYGON_OFFSET_FILL);

		glDisable(GL_TEXTURE_2D);

		if (blend)
			glDisable(GL_BLEND);

		/*
		 * For textures as panel widget backgrounds,
		 * draw the texture over a transparent block.
		 */
		fp = &fill_xparent;
	}

	blend = (fp->f_a != 1.0 && (fp->f_flags & FF_OPAQUE) == 0);

	if (blend) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glBegin(GL_QUADS);
	glColor4f(fp->f_r, fp->f_g, fp->f_b, blend ? fp->f_a : 1.0);
	glVertex2d(uoff + 1,			voff);
	glVertex2d(uoff + PWIDGET_LENGTH - 1,	voff);
	glVertex2d(uoff + PWIDGET_LENGTH - 1,	voff - PWIDGET_HEIGHT + 2);
	glVertex2d(uoff + 1,			voff - PWIDGET_HEIGHT + 2);
	glEnd();

	if (blend)
		glDisable(GL_BLEND);
}

__inline void
panel_calcwlens(struct panel *p)
{
	struct pwidget *pw;
	int nc, j, col;
	size_t siz;

	nc = MIN(p->p_nwcol, p->p_nwidgets);
	siz = sizeof(*p->p_wcw) * nc;
	if ((p->p_wcw = realloc(p->p_wcw, siz)) == NULL)
		err(1, "realloc");
	memset(p->p_wcw, 0, siz);
	j = 0;
	SLIST_FOREACH(pw, &p->p_widgets, pw_next) {
		col = j * nc / p->p_nwidgets;
		p->p_wcw[col] = MAX(p->p_wcw[col],
		    strlen(pw->pw_str));
		j++;
if (col > nc)
  errx(1, "col > nc");
	}
	p->p_totalwcw = 0;
	for (j = 0; j < nc; j++) {
		p->p_wcw[j] = MIN(PWLABEL_MAXLEN,
		    LETTER_WIDTH * (int)p->p_wcw[j] +
		    PWIDGET_LENGTH + 2 * PWIDGET_PADDING);
		p->p_totalwcw += p->p_wcw[j];
	}
	p->p_totalwcw -= PWIDGET_PADDING;
}

/*
 * The panel drawing API naming is horrible but is consistent:
 * draw_panel is the low-level function that draws an actual panel.
 * panel_draw is the high-level panel function that does what is
 * necessary to put a panel on the screen.
 */
void
draw_panel(struct panel *p, int toff)
{
	int contwidth, uoff, voff, stwv;
	int colwidth, col, nc, npw;
	struct fill *frame_fp;
	struct pwidget *pw;
	const char *s;

	/* Save state and set things up for 2D. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, winv.iv_w, 0.0, winv.iv_h);

	glColor4f(p->p_fill.f_r, p->p_fill.f_g, p->p_fill.f_b, p->p_fill.f_a);
	/* Panel content. */
	contwidth = 0;
	uoff = p->p_u + toff;
	voff = p->p_v - toff + 3;
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n' || s == p->p_str) {
			voff -= LETTER_HEIGHT;
			if (voff < p->p_v - p->p_h)
				break;
			glRasterPos2d(uoff, voff);
			contwidth = 0;
			if (*s == '\n')
				continue;
		}
		contwidth += LETTER_WIDTH;
		if (contwidth < p->p_w)
			glutBitmapCharacter(PFONT, *s);
	}

	/* Text shadow. */
	glColor3f(0.0, 0.0, 0.2);

	contwidth = 0;
	voff = p->p_v - toff + 3;
	uoff++;
	voff--;
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n' || s == p->p_str) {
			voff -= LETTER_HEIGHT;
			if (voff < p->p_v - p->p_h)
				break;
			glRasterPos2d(uoff, voff);
			contwidth = 0;
			if (*s == '\n')
				continue;
		}
		contwidth += LETTER_WIDTH;
		if (contwidth < p->p_w)
			glutBitmapCharacter(PFONT, *s);
	}
	uoff--;
	voff++;

	/* First loop cuts into these. */
//	uoff += p->p_w / 2 - toff;
	voff += PWIDGET_HEIGHT - 1 - PWIDGET_PADDING;
	stwv = voff;

	nc = MIN(p->p_nwcol, p->p_nwidgets);
	npw = 0;
	col = 0;
	colwidth = 0; /* gcc */
	SLIST_FOREACH(pw, &p->p_widgets, pw_next) {
		struct fill *fp = pw->pw_fillp;

		/* Starting a new column? */
		if (npw * nc / p->p_nwidgets != col) {
			col++;
			uoff += colwidth;
			voff = stwv;
if (col > nc)
  errx(1, "col > nc");
		}
		if (voff == stwv)
			colwidth = p->p_wcw[col] *
			    (p->p_w - 2 * toff) / p->p_totalwcw;
		voff -= PWIDGET_HEIGHT + PWIDGET_PADDING;
		if (voff - PWIDGET_HEIGHT < p->p_v - p->p_h) {
			pw->pw_flags |= PWF_HIDE;
			goto next;
		}
		pw->pw_flags &= ~PWF_HIDE;

		if ((p->p_info->pi_flags & PIF_XPARENT) == 0) {
			if (fp->f_flags & FF_SKEL)
				frame_fp = fp;
			else {
				frame_fp = &fill_black;

				/* Draw widget background. */
				draw_pwbg(fp, uoff, voff);
			}

			if (frame_fp->f_a) {
				/* Draw widget border. */
				glLineWidth(1.0f);
				glBegin(GL_LINE_LOOP);
				glColor4f(frame_fp->f_r, frame_fp->f_g,
				    frame_fp->f_b, frame_fp->f_a);
				glVertex2d(uoff,			voff);
				glVertex2d(uoff + PWIDGET_LENGTH,	voff);
				glVertex2d(uoff + PWIDGET_LENGTH - 1,	voff - PWIDGET_HEIGHT + 1);
				glVertex2d(uoff,			voff - PWIDGET_HEIGHT + 1);
				glEnd();
			}
		}

		glColor4f(p->p_fill.f_r, p->p_fill.f_g, p->p_fill.f_b, p->p_fill.f_a);
		glRasterPos2d(uoff + PWIDGET_LENGTH + PWIDGET_PADDING,
		    voff - PWIDGET_HEIGHT + 3);
		for (s = pw->pw_str; *s != '\0' &&
		    (s - pw->pw_str + 1) * LETTER_WIDTH +
		    PWIDGET_LENGTH + PWIDGET_PADDING < colwidth; s++)
			glutBitmapCharacter(PFONT, *s);

		/* Text shadow. */
		glColor4f(0.0, 0.0, 0.2, 0.0);
		glRasterPos2d(uoff + PWIDGET_LENGTH + PWIDGET_PADDING + 1,
		    voff - PWIDGET_HEIGHT + 3 - 1);
		for (s = pw->pw_str; *s != '\0' &&
		    (s - pw->pw_str + 1) * LETTER_WIDTH +
		    PWIDGET_LENGTH + PWIDGET_PADDING < colwidth; s++)
			glutBitmapCharacter(PFONT, *s);

		pw->pw_u = uoff;
		pw->pw_v = voff;
		pw->pw_w = PWIDGET_LENGTH + PWIDGET_PADDING +
		    (s - pw->pw_str) * LETTER_WIDTH;
		pw->pw_h = PWIDGET_HEIGHT;

next:
		npw++;
	}

	if ((p->p_info->pi_flags & PIF_XPARENT) == 0) {
		struct fill *fp;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		fp = (p->p_info->pi_flags & PIF_UINP) ? &fill_ipanel : &fill_panel;

		/* Draw background. */
		glBegin(GL_QUADS);
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		glVertex2d(p->p_u + PANEL_BWIDTH,		p->p_v + 1 - PANEL_BWIDTH);
		glVertex2d(p->p_u + p->p_w - PANEL_BWIDTH,	p->p_v + 1 - PANEL_BWIDTH);
		glVertex2d(p->p_u + p->p_w - PANEL_BWIDTH,	p->p_v + 1 - p->p_h + PANEL_BWIDTH);
		glVertex2d(p->p_u + PANEL_BWIDTH,		p->p_v + 1 - p->p_h + PANEL_BWIDTH);
		glEnd();

		glDisable(GL_BLEND);

		/* Draw border. */
		glLineWidth(PANEL_BWIDTH);
		glBegin(GL_LINE_LOOP);
		glColor4f(fill_panelbd.f_r, fill_panelbd.f_g,
		    fill_panelbd.f_b, fill_panelbd.f_a);
		glVertex2d(p->p_u,		p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v);
		glVertex2d(p->p_u + p->p_w - 1,	p->p_v - p->p_h + 1);
		glVertex2d(p->p_u,		p->p_v - p->p_h + 1);
		glEnd();
	}

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	if (p->p_extdrawf)
		p->p_extdrawf(p);
}

__inline void
make_panel(struct panel *p, int winid, int compile, int toff)
{
	if (compile) {
		if (p->p_dl[winid])
			glDeleteLists(p->p_dl[winid], 1);
		p->p_dl[winid] = glGenLists(1);
		glNewList(p->p_dl[winid], GL_COMPILE);
	}

	draw_panel(p, toff);

	if (compile) {
		glEndList();
		p->p_opts &= ~POPT_DIRTY;
		p->p_opts |= POPT_COMPILE;
		glCallList(p->p_dl[winid]);
	}
}

void
panel_draw(struct panel *p, int winid)
{
	int compile, tweenadj, u, v, w, h;
	int lineno, curlen;
	int toff;
	char *s;

	toff = PANEL_PADDING + PANEL_BWIDTH;

	/*
	 * The second window should see the same panel
	 * as the right, so skip calculation (otherwise,
	 * it would be slower and the contents may differ).
	 */
	if (stereo_mode == STM_PASV &&
	    p->p_opts & POPT_CANSYNC) {
		/*
		 * make_panel() on the first window
		 * will have cleared the dirty bit.
		 */
		make_panel(p, winid, p->p_opts & POPT_COMPILE, toff);
		p->p_opts &= ~(POPT_COMPILE | POPT_CANSYNC);
		return;
	}

	if ((p->p_opts & POPT_DIRTY) == 0 && p->p_dl[winid]) {
		glCallList(p->p_dl[winid]);
		goto done;
	}
	p->p_opts |= POPT_CANSYNC;

	if (p->p_opts & POPT_REMOVE) {
		w = 0;
		h = 0;
		switch (p->p_info->pi_stick) {
		case PSTICK_TL:
			u = -p->p_w;
			v = winv.iv_h - panel_offset[PVOFF_TL];
			break;
		case PSTICK_BL:
			u = -p->p_w;
			v = panel_offset[PVOFF_BL];
			break;
		case PSTICK_TR:
			u = winv.iv_w;
			v = winv.iv_h - panel_offset[PVOFF_TR];
			break;
		case PSTICK_BR:
			u = winv.iv_w;
			v = panel_offset[PVOFF_BR];
			break;
		default:
			u = p->p_u;
			v = p->p_v;
			break;
		}
	} else {
		lineno = 1;
		curlen = w = 0;
		for (s = p->p_str; *s != '\0'; s++, curlen++) {
			if (*s == '\n') {
				if (curlen > w)
					w = curlen;
				curlen = -1;
				lineno++;
			}
		}
		/* Let's hope there are some pwidgets... */
		if (p->p_str[0] == '\0')
			lineno = 0;
		if (curlen > w)
			w = curlen;
		w = w * LETTER_WIDTH;
		h = lineno * LETTER_HEIGHT + 2 * toff;
		if (p->p_nwidgets) {
			int nr, nc;

			nc = MIN(p->p_nwcol, p->p_nwidgets);
			w = MAX(w, p->p_totalwcw);
			nr = (p->p_nwidgets + nc - 1) / nc;
			h += nr * (PWIDGET_HEIGHT + PWIDGET_PADDING);
		}
		w += 2 * toff;
		switch (p->p_info->pi_stick) {
		case PSTICK_TL:
			u = 0;
			v = (winv.iv_h - 1) - panel_offset[PVOFF_TL];
			break;
		case PSTICK_BL:
			u = 0;
			v = panel_offset[PVOFF_BL] + p->p_h - 1;
			break;
		case PSTICK_TR:
			u = winv.iv_w - w;
			v = (winv.iv_h - 1) - panel_offset[PVOFF_TR];
			break;
		case PSTICK_BR:
			u = winv.iv_w - w;
			v = panel_offset[PVOFF_BR] + p->p_h - 1;
			break;
		default:
			u = p->p_u;
			v = p->p_v;
			break;
		}
	}
	if (p->p_opts & POPT_MOBILE) {
		u = p->p_u;
		v = p->p_v;
	}

	tweenadj = MAX(abs(p->p_u - u), abs(p->p_v - v));
	tweenadj = MAX(tweenadj, abs(p->p_w - w));
	tweenadj = MAX(tweenadj, abs(p->p_h - h));
	if (tweenadj) {
		tweenadj = sqrt(tweenadj);
		if (!tweenadj)
			tweenadj = 1;
		/*
		 * The panel is being animated, so don't waste
		 * time recompiling it.
		 */
		compile = 0;
	} else
		compile = 1;

	if (p->p_u != u)
		p->p_u -= (p->p_u - u) / tweenadj;
	if (p->p_v != v)
		p->p_v -= (p->p_v - v) / tweenadj;
	if (p->p_w != w)
		p->p_w -= (p->p_w - w) / tweenadj;
	if (p->p_h != h)
		p->p_h -= (p->p_h - h) / tweenadj;

	if (p->p_opts & POPT_REMOVE) {
		if ((p->p_info->pi_stick == PSTICK_TL && p->p_u + p->p_w <= 0) ||
		    (p->p_info->pi_stick == PSTICK_BL && p->p_u + p->p_w <= 0) ||
		    (p->p_info->pi_stick == PSTICK_TR && p->p_u >= winv.iv_w) ||
		    (p->p_info->pi_stick == PSTICK_BR && p->p_u >= winv.iv_w)) {
			panel_free(p);
			return;
		}
	}

	make_panel(p, winid, compile, toff);

done:
	/* spacing */
	switch (p->p_info->pi_stick) {
	case PSTICK_TL:
		panel_offset[PVOFF_TL] += p->p_h + 1;
		break;
	case PSTICK_TR:
		panel_offset[PVOFF_TR] += p->p_h + 1;
		break;
	case PSTICK_BL:
		panel_offset[PVOFF_BL] += p->p_h + 1;
		break;
	case PSTICK_BR:
		panel_offset[PVOFF_BR] += p->p_h + 1;
		break;
	}
}

void
draw_panels(int winid)
{
	struct panel *p, *np;

	/*
	 * Can't use TAILQ_FOREACH() since draw_panel() may remove
	 * a panel.
	 */
	memset(panel_offset, 0, sizeof(panel_offset));
	for (p = TAILQ_FIRST(&panels); p != TAILQ_END(&panels); p = np) {
		np = TAILQ_NEXT(p, p_link);
		if (stereo_mode != STM_PASV ||
		   (p->p_opts & POPT_CANSYNC) == 0)
			p->p_refresh(p);
		panel_draw(p, winid);
	}
}

struct panel *
panel_for_id(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id)
			return (p);
	return (NULL);
}

void
panel_show(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id) {
			if (p->p_opts & POPT_REMOVE)
				panel_tremove(p);
			return;
		}
	panel_toggle(id);
}

void
panel_hide(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id) {
			if ((p->p_opts & POPT_REMOVE) == 0)
				panel_tremove(p);
			return;
		}
}

void
panel_tremove(struct panel *p)
{
	struct panel *t;

	if (p->p_info->pi_flags & PIF_UINP && uinp.uinp_panel != NULL) {
		buf_reset(&uinp.uinp_buf);
		buf_append(&uinp.uinp_buf, '\0');
		glutKeyboardFunc(gl_keyh_default);
		uinp.uinp_panel = NULL;
	}

	/* XXX inspect POPT_REMOVE first? */
	p->p_opts ^= POPT_REMOVE;
	TAILQ_FOREACH(t, &panels, p_link)
		if (t->p_info->pi_stick == p->p_info->pi_stick)
			t->p_opts |= POPT_DIRTY;
	if (flyby_mode == FBM_REC)
		flyby_writepanel(p->p_id);
}

void
panel_toggle(int panel)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == panel) {
			panel_tremove(p);
			return;
		}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	memset(p, 0, sizeof(*p));

	p->p_info = &pinfo[baseconv(panel) - 1];
	p->p_refresh = p->p_info->pi_refresh;
	p->p_id = panel;
	p->p_fill.f_r = 1.0f;
	p->p_fill.f_g = 1.0f;
	p->p_fill.f_b = 1.0f;
	p->p_fill.f_a = 1.0f;
	SLIST_INIT(&p->p_widgets);

	switch (p->p_info->pi_stick) {
	case PSTICK_TL:
		p->p_u = 0;
		p->p_v = (winv.iv_h - 1) - panel_offset[PVOFF_TL];
		break;
	case PSTICK_TR:
		p->p_u = winv.iv_w;
		p->p_v = (winv.iv_h - 1) - panel_offset[PVOFF_TR];
		break;
	case PSTICK_BL:
		p->p_u = 0;
		p->p_v = panel_offset[PVOFF_BL] - 1;
		break;
	case PSTICK_BR:
		p->p_u = winv.iv_w;
		p->p_v = panel_offset[PVOFF_BR] - 1;
		break;
	}

	if (p->p_info->pi_flags & PIF_UINP) {
		if (uinp.uinp_panel != NULL) {
			struct panel *oldp;

			buf_reset(&uinp.uinp_buf);
			buf_append(&uinp.uinp_buf, '\0');
			oldp = uinp.uinp_panel;
			uinp.uinp_panel = NULL;
			panel_tremove(oldp);
		}

		glutKeyboardFunc(gl_keyh_uinput);
		uinp.uinp_panel = p;
		uinp.uinp_opts = p->p_info->pi_uinpopts;
		uinp.uinp_callback = p->p_info->pi_uinpcb;
	}
	TAILQ_INSERT_TAIL(&panels, p, p_link);
	if (flyby_mode == FBM_REC)
		flyby_writepanel(p->p_id);
}

void
panel_demobilize(struct panel *p)
{
	struct panel *t;

	TAILQ_FOREACH(t, &panels, p_link)
		if (t->p_info->pi_stick == p->p_info->pi_stick)
			t->p_opts |= POPT_DIRTY;

	p->p_opts &= ~POPT_MOBILE;
	p->p_opts |= POPT_DIRTY;
	if (mousev.iv_u < winv.iv_w / 2) {
		if (mousev.iv_v < winv.iv_h / 2)
			p->p_info->pi_stick = PSTICK_TL;
		else
			p->p_info->pi_stick = PSTICK_BL;
	} else {
		if (mousev.iv_v < winv.iv_h / 2)
			p->p_info->pi_stick = PSTICK_TR;
		else
			p->p_info->pi_stick = PSTICK_BR;
	}

	TAILQ_FOREACH(t, &panels, p_link)
		if (t->p_info->pi_stick == p->p_info->pi_stick)
			t->p_opts |= POPT_DIRTY;
}

__inline void
panel_rebuild(int pids)
{
	struct panel *p;
	int b;

	while (pids) {
		b = ffs(pids) - 1;
		pids &= ~(1 << b);
		if ((p = panel_for_id(1 << b)) != NULL)
			p->p_opts |= POPT_REFRESH;
	}
}

__inline void
panel_redraw(int pids)
{
	struct panel *p;
	int b;

	while (pids) {
		b = ffs(pids) - 1;
		pids &= ~(1 << b);
		if ((p = panel_for_id(1 << b)) != NULL)
			p->p_opts |= POPT_DIRTY;
	}
}

void
panels_flip(int ps)
{
	int b;

	while (ps) {
		b = ffs(ps) - 1;
		ps &= ~(1 << b);
		panel_toggle(1 << b);
	}
}

/*
 * Show only the specified panels.
 */
void
panels_set(int set)
{
	struct panel *p;
	int j, diff, cur;

	cur = 0;
	TAILQ_FOREACH(p, &panels, p_link)
		cur |= p->p_id;
	diff = cur ^ set;
	for (j = 0; j < NPANELS; j++)
		if (pinfo[j].pi_flags & PIF_FBIGN)
			diff &= ~(1 << j);
	panels_flip(diff);
}

void
panels_show(int ps)
{
	int b;

	while (ps) {
		b = ffs(ps) - 1;
		ps &= ~(1 << b);
		panel_show(1 << b);
	}
}

void
panels_hide(int ps)
{
	int b;

	while (ps) {
		b = ffs(ps) - 1;
		ps &= ~(1 << b);
		panel_hide(1 << b);
	}
}

void
panel_draw_compass(struct panel *p)
{
	int voff;

	/* Special case: remove space for panel title. */
	voff = -7;
	if (p->p_h - abs(voff) <= 0)
		return;
	draw_compass(
	    p->p_u + p->p_w / 2, p->p_w,
	    p->p_v - p->p_h / 2 + voff, p->p_h - abs(voff));
}
