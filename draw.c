/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdlib.h>

#include "cdefs.h"
#include "cam.h"
#include "capture.h"
#include "draw.h"
#include "env.h"
#include "fill.h"
#include "flyby.h"
#include "node.h"
#include "panel.h"
#include "queue.h"
#include "route.h"
#include "selnode.h"
#include "state.h"
#include "tween.h"
#include "xmath.h"

#define SELNODE_GAP	(0.1f)

#define TEXTURE_SIZE	128.0

#define FONT_WIDTH	12.0
#define FONT_HEIGHT	12.0
#define FONT_TEX_W 	256.0
#define FONT_TEX_H 	16.0

/* How many units of texture coordinates a character displaces */
#define FONT_TEXCOORD_S (1 / (FONT_TEX_W / FONT_WIDTH))
#define FONT_TEXCOORD_T (1 / (FONT_TEX_H / FONT_HEIGHT))

/* How many pixels a character displaces on a 128x128 tile */
#define FONT_DISPLACE_W ((FONT_WIDTH * NODEDEPTH) / TEXTURE_SIZE * 2)
#define FONT_DISPLACE_H ((FONT_HEIGHT * NODEHEIGHT) / TEXTURE_SIZE * 2)

#define MAX_CHARS 	4
#define FONT_Z_OFFSET	((NODEHEIGHT - ((MAX_CHARS + 0) * FONT_DISPLACE_W)) / 2)

#define SKEL_GAP (0.1f)

#define FOCAL_POINT (2.0f) /* distance from cam to center of 3d focus */
#define FOCAL_LENGTH (5.0f) /* length of 3d focus */

float	 snap_to_grid(float, float, float);

struct fvec wi_repstart;
struct fvec wi_repdim;
float clip;

int dl_cluster[2];
int dl_ground[2];
int dl_selnodes[2];

__inline void
wired_update(void)
{
	if (st.st_x + clip > wi_repstart.fv_x + wi_repdim.fv_w ||
	    st.st_x - clip < wi_repstart.fv_x ||
	    st.st_y + clip > wi_repstart.fv_y + wi_repdim.fv_h ||
	    st.st_y - clip < wi_repstart.fv_y ||
	    st.st_z + clip > wi_repstart.fv_z + wi_repdim.fv_d ||
	    st.st_z - clip < wi_repstart.fv_z) {
//		status_add("Rebuild triggered\n");
		st.st_rf |= RF_CLUSTER;
	}
}

void
draw_compass(void)
{
	struct fvec fv, upadj, normv;
	GLUquadric *q;

	cam_getspecvec(&fv, 150, winv.iv_h - 150);
	fv.fv_x *= 2.0;
	fv.fv_y *= 2.0;
	fv.fv_z *= 2.0;
	vec_addto(&st.st_v, &fv);

	vec_set(&normv, 0.0, 1.0, 0.0);
	vec_sub(&upadj, &normv, &st.st_uv);

	glPushMatrix();
//	glTranslatef(upadj.fv_x, 0.0, upadj.fv_z);

	if ((q = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");
	gluQuadricDrawStyle(q, GLU_FILL);

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

#define CMP_LLEN (0.25)
#define CMP_ATHK (0.0125)
#define CMP_ALEN (0.025)

	glColor3f(0.0f, 1.0f, 0.0f);					/* z - green */
	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z - CMP_LLEN / 2.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, CMP_LLEN);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z + CMP_LLEN / 2.0);
	gluCylinder(q, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glColor3f(1.0f, 0.0f, 0.0f);					/* y - red */
	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y - CMP_LLEN / 2.0, fv.fv_z);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, CMP_LLEN, 0.0);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y + CMP_LLEN / 2.0, fv.fv_z);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(q, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glColor3f(0.0f, 0.0f, 1.0f);					/* x - blue */
	glPushMatrix();
	glTranslatef(fv.fv_x - CMP_LLEN / 2.0, fv.fv_y, fv.fv_z);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(CMP_LLEN, 0.0, 0.0);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(fv.fv_x + CMP_LLEN / 2.0, fv.fv_y, fv.fv_z);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	gluCylinder(q, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glPopMatrix();

	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
	gluDeleteQuadric(q);
}

__inline void
draw_scene(void)
{
	if (dl_selnodes[wid])
		glCallList(dl_selnodes[wid]);
	glCallList(dl_cluster[wid]);
	if (st.st_opts & OP_GROUND)
		glCallList(dl_ground[wid]);
	if (!TAILQ_EMPTY(&panels))
		draw_panels(wid);
// draw_compass();
// job_drawlabels();
}

void
gl_displayh_stereo(void)
{
	static double ratio, radians, wd2, ndfl, eyesep;
	static float left, right, top, bottom;
	static struct fvec stereo_fv;
	int rf, newrf;

	if (flyby_mode)
		flyby_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	rf = st.st_rf;
	if (rf) {
		st.st_rf = 0;
		rebuild(rf);
	}

	ratio = ASPECT;
	radians = DEG_TO_RAD(FOVY / 2);
	wd2 = FOCAL_POINT * tan(radians);
	ndfl = FOCAL_POINT / FOCAL_LENGTH;
	eyesep = 0.30f;

	vec_crossprod(&stereo_fv, &st.st_lv, &st.st_uv);
	vec_normalize(&stereo_fv);
	stereo_fv.fv_x *= eyesep / 2.0f;
	stereo_fv.fv_y *= eyesep / 2.0f;
	stereo_fv.fv_z *= eyesep / 2.0f;

	/* Draw right buffer. */
	switch (stereo_mode) {
	case STM_ACT:
		glDrawBuffer(GL_BACK_RIGHT);
		break;
	case STM_PASV:
		wid = WINID_RIGHT;
		glutSetWindow(window_ids[wid]);
		break;
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	left = -ratio * wd2 - eyesep * ndfl / 2;
	right = ratio * wd2 - eyesep * ndfl / 2;
	top = wd2;
	bottom = -wd2;
	glFrustum(left, right, bottom, top, NEARCLIP, clip);

	glMatrixMode(GL_MODELVIEW);
	st.st_x += stereo_fv.fv_x;
	st.st_y += stereo_fv.fv_y;
	st.st_z += stereo_fv.fv_z;
	cam_look();

	newrf = st.st_rf;
	st.st_rf = rf;
	draw_scene();

	if (stereo_mode == STM_PASV) {
		glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
		/* XXX: capture frame */
		if (st.st_opts & OP_CAPTURE)
			capture_frame(capture_mode);
		if (st.st_opts & OP_DISPLAY)
			glutSwapBuffers();
	}

	/* Draw left buffer. */
	switch (stereo_mode) {
	case STM_ACT:
		glDrawBuffer(GL_BACK_LEFT);
		break;
	case STM_PASV:
		wid = WINID_LEFT;
		glutSetWindow(window_ids[wid]);
		break;
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	left = -ratio * wd2 + eyesep * ndfl / 2;
	right = ratio * wd2 + eyesep * ndfl / 2;
	top = wd2;
	bottom = -wd2;
	glFrustum(left, right, bottom, top, NEARCLIP, clip);

	glMatrixMode(GL_MODELVIEW);
	st.st_x -= 2 * stereo_fv.fv_x;
	st.st_y -= 2 * stereo_fv.fv_y;
	st.st_z -= 2 * stereo_fv.fv_z;
	cam_look();

	draw_scene();
	st.st_rf = newrf;

	glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
	if (st.st_opts & OP_CAPTURE)
		capture_frame(capture_mode);
	if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();

	/* Restore camera position. */
	st.st_x += stereo_fv.fv_x;
	st.st_y += stereo_fv.fv_y;
	st.st_z += stereo_fv.fv_z;
}

void
gl_displayh_default(void)
{
	int rf, newrf;

	if (flyby_mode)
		flyby_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	rf = st.st_rf;
	if (rf) {
		st.st_rf = 0;
		rebuild(rf);
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

	glClearColor(fill_bg.f_r, fill_bg.f_g, fill_bg.f_b, 1.0);
	if (st.st_opts & OP_CAPTURE)
		capture_frame(capture_mode);
	if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();
}

/* Render a character from the font texture. */
__inline void
draw_char(int ch, float x, float y, float z)
{
	if (ch < 0)
		return;

	glTexCoord2f(FONT_TEXCOORD_S * ch, 0.0);
	glVertex3f(x, y, z);

	glTexCoord2f(FONT_TEXCOORD_S * ch, FONT_TEXCOORD_T);
	glVertex3f(x, y + FONT_DISPLACE_H, z);

	glTexCoord2f(FONT_TEXCOORD_S * (ch + 1), FONT_TEXCOORD_T);
	glVertex3f(x, y + FONT_DISPLACE_H, z + FONT_DISPLACE_W);

	glTexCoord2f(FONT_TEXCOORD_S * (ch + 1), 0.0);
	glVertex3f(x, y, z + FONT_DISPLACE_W);
}

__inline void
draw_node_label(struct node *n)
{
	float list[MAX_CHARS];
	struct fill c;
	int i, nid;

	/*
	 * Parse the node id for use with
	 * NODE0123456789 (so 4 letter gap before 0)
	 */
	i = 0;
	nid = n->n_nid;
	while (nid >= 0 && i < MAX_CHARS) {
		list[MAX_CHARS - i - 1] = 4 + nid % 10;
		nid /= 10;
		i++;
	}

	while (i < MAX_CHARS)
		list[i++] = -1;

	glEnable(GL_TEXTURE_2D);

	/* Get a distinct contrast color */
	c = *n->n_fillp;
	if ((c.f_flags & FF_SKEL) == 0)
		fill_contrast(&c);
	glColor4f(c.f_r, c.f_g, c.f_b, c.f_a);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, fill_font.f_texid_a[wid]);

	glBegin(GL_QUADS);

	for (i = 0; i < MAX_CHARS; i++) {
		/* -0.001 to place slightly in front */
		draw_char(list[i], -0.001, NODEDEPTH / 2.0,
		    FONT_Z_OFFSET + FONT_DISPLACE_W * (float)i);
	}

	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

/*
 * Special case of pipe-drawing code: draw pipes around selected node
 * only.
 */
__inline void
draw_node_pipes(struct node *np)
{
	struct fvec *ndim;
	GLUquadric *q;

	ndim = &vmodes[st.st_vmode].vm_ndim;

	if ((q = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");
	gluQuadricDrawStyle(q, GLU_FILL);

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	glColor4f(0.0f, 1.0f, 0.0f, 0.7f);				/* z - green */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + ndim->fv_w / 2.0,
	    np->n_v->fv_y + ndim->fv_h / 2.0,
	    np->n_v->fv_z + ndim->fv_d / 2.0 - st.st_winsp.iv_d);
	gluCylinder(q, 0.1, 0.1, 2.0 * st.st_winsp.iv_d, 3, 1);
	glPopMatrix();

	glColor4f(1.0f, 0.0f, 0.0f, 0.7f);				/* y - red */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + ndim->fv_w / 2.0,
	    np->n_v->fv_y + ndim->fv_h / 2.0 - st.st_winsp.iv_h,
	    np->n_v->fv_z + ndim->fv_d / 2.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(q, 0.1, 0.1, 2.0 * st.st_winsp.iv_h, 3, 1);
	glPopMatrix();

	glColor4f(0.0f, 0.0f, 1.0f, 0.7f);				/* x - blue */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + ndim->fv_w / 2.0 - st.st_winsp.iv_w,
	    np->n_v->fv_y + ndim->fv_h / 2.0,
	    np->n_v->fv_z + ndim->fv_d / 2.0);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	gluCylinder(q, 0.1, 0.1, 2.0 * st.st_winsp.iv_w, 3, 1);
	glPopMatrix();

	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_BLEND);
	gluDeleteQuadric(q);
}

__inline int
node_tween_dir(float *curpos, float *targetpos)
{
	float diff;
	int rv = 0;

	diff = *targetpos - *curpos;
	if (diff < 0.1 && diff > -0.1)
		*curpos = *targetpos;
	else {
		if (diff > 2.0 || diff < -2.0)
			diff = 2 * SIGN(diff) * sqrt(fabs(diff));
		else
			diff /= 5 / 7.0f;
		*curpos += diff;
		rv = 1;
	}
	return (rv);
}

__inline void
draw_node(struct node *n, int flags)
{
	struct fill *fp, *fill_wireframe;
	struct fvec *dimp, *fvp;

	GLenum param = GL_REPLACE;

	if (!node_show(n))
		return;

	fp = n->n_fillp;
	dimp = &vmodes[st.st_vmode].vm_ndim;

	/*
	 * We assume that the stack we are in otherwise
	 * has done the translation itself.
	 */
	if ((flags & NDF_DONTPUSH) == 0) {
		glPushMatrix();

		if (st.st_opts & OP_NODEANIM &&
		    node_tween_dir(&n->n_vcur.fv_x, &n->n_v->fv_x) |
		    node_tween_dir(&n->n_vcur.fv_y, &n->n_v->fv_y) |
		    node_tween_dir(&n->n_vcur.fv_z, &n->n_v->fv_z)) {
			st.st_rf |= RF_CLUSTER;
			fvp = &n->n_vcur;
		} else
			fvp = n->n_v;
		glTranslatef(fvp->fv_x, fvp->fv_y, fvp->fv_z);
	}

	if (fp->f_a != 1.0f) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		param = GL_BLEND;
	}

	if (n->n_fillp->f_flags & FF_SKEL)
		fill_wireframe = n->n_fillp;
	else {
		fill_wireframe = &fill_black;

		if (st.st_opts & OP_TEX)
			draw_box_tex(dimp, fp, param);
		else
			draw_box_filled(dimp, fp);
	}

	if (fp->f_a != 1.0f)
		glDisable(GL_BLEND);

	if (st.st_opts & OP_WIREFRAME) {
		float col;

		col = fill_wireframe->f_a;
		fill_wireframe->f_a = fp->f_a;
		draw_box_outline(dimp, fill_wireframe);
		fill_wireframe->f_a = col;
	}
	if (st.st_opts & OP_NLABELS)
		draw_node_label(n);

	if ((flags & NDF_DONTPUSH) == 0)
		glPopMatrix();
}

__inline void
draw_mod(struct fvec *vp, struct fvec *dim, struct fill *fillp)
{
	struct fvec v;

	v = *vp;
	v.fv_x -= 0.01;
	v.fv_y -= 0.01;
	v.fv_z -= 0.01;
	glPushMatrix();
	glTranslatef(v.fv_x, v.fv_y, v.fv_z);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
	draw_box_filled(dim, fillp);
	glDisable(GL_BLEND);

	if (st.st_opts & OP_WIREFRAME)
		draw_box_outline(dim, &fill_black);
	glPopMatrix();
}

void
make_ground(void)
{
	struct fvec fv, fdim;
	struct fill fill;

	if (dl_ground[wid])
		glDeleteLists(dl_ground[wid], 1);
	dl_ground[wid] = glGenLists(1);
	glNewList(dl_ground[wid], GL_COMPILE);

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(1.0);
	glBegin(GL_LINES);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex3f(-500.0f, 0.0f, 0.0f);		/* x-axis */
	glVertex3f(500.0f, 0.0f, 0.0f);
	glColor3f(0.6f, 0.6f, 1.0f);
	glVertex3f(0.0f, -500.0f, 0.0f);		/* y-axis */
	glVertex3f(0.0f, 500.0f, 0.0f);
	glColor3f(1.0f, 0.9f, 0.0f);
	glVertex3f(0.0f, 0.0f, -500.0f);		/* z-axis */
	glVertex3f(0.0f, 0.0f, 500.0f);
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);

	/* Ground */
	fill.f_r = 0.3f;
	fill.f_g = 0.3f;
	fill.f_b = 0.3f;
	fill.f_a = 0.1f;
	switch (st.st_vmode) {
	case VM_WIREDONE:
		fv.fv_x = st.st_winsp.iv_x * (-0.5f + st.st_wioff.iv_x);
		fv.fv_y = -0.2f + st.st_wioff.iv_y * st.st_winsp.iv_y;
		fv.fv_z = st.st_winsp.iv_z * (-0.5f + st.st_wioff.iv_z);

		fdim.fv_w = WIV_SWIDTH + st.st_winsp.iv_x / 2.0f;
		fdim.fv_y = -0.2f / 2.0f;
		fdim.fv_d = WIV_SDEPTH + st.st_winsp.iv_z / 2.0f;

		glPushMatrix();
		glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

		draw_box_filled(&fdim, &fill);
		draw_box_outline(&fdim, &fill_black);

		glDisable(GL_BLEND);
		glPopMatrix();
		break;
	case VM_PHYSICAL:
		fv.fv_x = -5.0f;
		fv.fv_y = -0.2f;
		fv.fv_z = -5.0f;

		fdim.fv_w = ROWWIDTH - 2 * fv.fv_x;
		fdim.fv_h = -fv.fv_y / 2.0f;
		fdim.fv_d = 2 * ROWDEPTH + ROWSPACE - 2 * fv.fv_z;

		glPushMatrix();
		glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

		draw_box_filled(&fdim, &fill);
		draw_box_outline(&fdim, &fill_black);

		glDisable(GL_BLEND);
		glPopMatrix();
		break;
	}

	glEndList();
}

#if 0
void
draw_skel(struct fvec *fv, struct fvec *dim)
{
	for (pd = LIST_FIRST(&physdims); pd != NULL; pd = pd_contains) {
		if (pd->pd_flags & PDF_SKEL) {
			for () {
				glPushMatrix();
				glTranslatef(v.fv_x - SKEL_GAP,
				    v.fv_y - SKEL_GAP, v.fv_z - SKEL_GAP);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

				fill_light_blue.f_a = 0.4f;
				draw_box_outline(&skel, &fill_light_blue);
				fill_light_blue.f_a = 1.0f;

				glDisable(GL_BLEND);
				glPopMatrix();
			}
		}
	}
}
#endif

__inline void
draw_cluster_physical(void)
{
	struct fvec v, mdim, skel;
	int r, cb, cg, m, n;
	struct node *node;
	struct fill mf;

	mdim.fv_w = MODWIDTH + 0.02;
	mdim.fv_h = MODHEIGHT + 0.02;
	mdim.fv_d = MODDEPTH + 0.02;

	mf.f_r = 1.00;
	mf.f_g = 1.00;
	mf.f_b = 1.00;
	mf.f_a = 0.20;

	skel.fv_w = CABWIDTH  + 2 * SKEL_GAP;
	skel.fv_h = CABHEIGHT + 2 * SKEL_GAP;
	skel.fv_z = MODDEPTH  + 2 * SKEL_GAP;

	v.fv_x = v.fv_y = v.fv_z = NODESPACE;
#if 0
	for (iv.iv_x = 0; iv.iv_x < WIDIM_WIDTH; iv.iv_x++)
		for (iv.iv_y = 0; iv.iv_y < WIDIM_HEIGHT; iv.iv_y++)
			for (iv.iv_z = 0; iv.iv_z < WIDIM_DEPTH; iv.iv_z++) {
				n = &invmap[iv.iv_x][iv.iv_y][iv.iv_z];
				n->n_v = &n->n_physv;
				node_setphyspos(n, &node->n_physv);

				node_adjmodpos(node->, &node->n_physv);
				draw_node(node, 0);
			}
#endif

	for (r = 0; r < NROWS; r++, v.fv_z += ROWDEPTH + ROWSPACE) {
		for (cb = 0; cb < NCABS; cb++, v.fv_x += CABWIDTH + CABSPACE) {
			for (cg = 0; cg < NCAGES; cg++, v.fv_y += CAGEHEIGHT + CAGESPACE) {
				for (m = 0; m < NMODS; m++, v.fv_x += MODWIDTH + MODSPACE) {
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_v = &node->n_physv;
						node->n_physv = v;
						node_adjmodpos(n, &node->n_physv);
						draw_node(node, 0);
					}
					if (st.st_opts & OP_SHOWMODS)
						draw_mod(&v, &mdim, &mf);
				}
				v.fv_x -= (MODWIDTH + MODSPACE) * NMODS;
			}
			v.fv_y -= (CAGEHEIGHT + CAGESPACE) * NCAGES;
			if (st.st_opts & OP_SKEL) {
				glPushMatrix();
				glTranslatef(v.fv_x - SKEL_GAP,
				    v.fv_y - SKEL_GAP, v.fv_z - SKEL_GAP);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

				fill_light_blue.f_a = 0.4f;
				draw_box_outline(&skel, &fill_light_blue);
				fill_light_blue.f_a = 1.0f;

				glDisable(GL_BLEND);
				glPopMatrix();
			}
		}
		v.fv_x -= (CABWIDTH + CABSPACE) * NCABS;
	}
}

struct fill rtclasses[] = {
 /*   0-10% */ FILL_INITA(1.0f, 1.0f, 0.4f, 0.5f),
 /*  10-20% */ FILL_INITA(1.0f, 0.9f, 0.4f, 0.5f),
 /*  20-30% */ FILL_INITA(1.0f, 0.8f, 0.4f, 0.5f),
 /*  30-40% */ FILL_INITA(1.0f, 0.7f, 0.4f, 0.5f),
 /*  40-50% */ FILL_INITA(1.0f, 0.6f, 0.4f, 0.5f),
 /*  50-60% */ FILL_INITA(1.0f, 0.5f, 0.4f, 0.6f),
 /*  60-70% */ FILL_INITA(1.0f, 0.4f, 0.4f, 0.7f),
 /*  70-80% */ FILL_INITA(1.0f, 0.3f, 0.4f, 0.8f),
 /*  80-90% */ FILL_INITA(1.0f, 0.2f, 0.4f, 0.9f),
 /* 90-100% */ FILL_INITA(1.0f, 0.1f, 0.4f, 1.0f),
};

void
draw_cluster_pipe(GLUquadric *q, struct ivec *iv,
    struct fvec *sv, struct fvec *dimv)
{
	struct fill *fp, dims[] = {
		FILL_INITA(0.0f, 0.0f, 1.0f, 0.5f),	/* x - blue */
		FILL_INITA(1.0f, 0.0f, 0.0f, 0.5f),	/* y - red */
		FILL_INITA(0.0f, 1.0f, 0.0f, 0.5f)	/* z - green */
	};
	int *j, jmax, dim, port, class;
	struct ivec *spv;
	struct node *n;
	float len;

	if (dimv->fv_w)
		dim = DIM_X;
	else if (dimv->fv_h)
		dim = DIM_Y;
	else
		dim = DIM_Z;

	spv = &st.st_winsp;

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	switch (st.st_pipemode) {
	case PM_DIR:
		fp = &dims[dim];
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		glPushMatrix();

		switch (dim) {
		case DIM_X:
			glTranslatef(
			    sv->fv_x - spv->iv_x,
			    sv->fv_y + iv->iv_y * spv->iv_y,
			    sv->fv_z + iv->iv_z * spv->iv_z);
			glRotatef(90.0, 0.0, 1.0, 0.0);
			len = (widim.iv_w + 1) * st.st_winsp.iv_x;
			break;
		case DIM_Y:
			glTranslatef(
			    sv->fv_x + iv->iv_x * spv->iv_x,
			    sv->fv_y - spv->iv_y,
			    sv->fv_z + iv->iv_z * spv->iv_z);
			glRotatef(-90.0, 1.0, 0.0, 0.0);
			len = (widim.iv_h + 1) * st.st_winsp.iv_y;
			break;
		case DIM_Z:
			glTranslatef(
			    sv->fv_x + iv->iv_x * spv->iv_x,
			    sv->fv_y + iv->iv_y * spv->iv_y,
			    sv->fv_z - spv->iv_z);
			len = (widim.iv_d + 1) * st.st_winsp.iv_z;
			break;
		}
		gluCylinder(q, 0.1, 0.1, len, 3, 1);
		glPopMatrix();
		break;
	case PM_RT:
		switch (dim) {
		case DIM_X:
			jmax = widim.iv_w;
			j = &iv->iv_x;
			len = 2 * st.st_winsp.iv_x;
			break;
		case DIM_Y:
			jmax = widim.iv_h;
			j = &iv->iv_y;
			len = 2 * st.st_winsp.iv_y;
			break;
		case DIM_Z:
			jmax = widim.iv_d;
			j = &iv->iv_z;
			len = 2 * st.st_winsp.iv_z;
			break;
		}

		for (*j = 0; *j < jmax; *j++) {
			if ((n = wimap[iv->iv_x][iv->iv_y][iv->iv_z]) == NULL ||
			    !node_show(n))
				continue;

			port = DIM_TO_PORT(dim, rt_portset);
			if (n->n_route.rt_err[port][rt_type] == 0)
				continue;

			if (rt_max.rt_err[port][rt_type])
				class = 100 * n->n_route.rt_err[port][rt_type] /
				    rt_max.rt_err[port][rt_type];
			else
				class = 100;		/* 100% */

			fp = &rtclasses[class / 10];

			glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
			glPushMatrix();

			switch (dim) {
			case DIM_X:
				glTranslatef(
				    sv->fv_x + (iv->iv_x - 1) * spv->iv_x,
				    sv->fv_y + iv->iv_y * spv->iv_y,
				    sv->fv_z + iv->iv_z * spv->iv_z);
				glRotatef(90.0, 0.0, 1.0, 0.0);
				break;
			case DIM_Y:
				glTranslatef(
				    sv->fv_x + iv->iv_x * spv->iv_x,
				    sv->fv_y + (iv->iv_y - 1) * spv->iv_y,
				    sv->fv_z + iv->iv_z * spv->iv_z);
				glRotatef(-90.0, 1.0, 0.0, 0.0);
				break;
			case DIM_Z:
				glTranslatef(
				    sv->fv_x + iv->iv_x * spv->iv_x,
				    sv->fv_y + iv->iv_y * spv->iv_y,
				    sv->fv_z + (iv->iv_z - 1) * spv->iv_z);
				break;
			}

			gluCylinder(q, 0.1, 0.1, len, 3, 1);
			glPopMatrix();
		}
		break;
	}
	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);
}

__inline void
draw_cluster_pipes(const struct fvec *v)
{
	struct fvec *ndim, sv, dim;
	struct ivec iv;
	GLUquadric *q;

	glPushMatrix();
	glTranslatef(v->fv_x, v->fv_y, v->fv_z);

	ndim = &vmodes[st.st_vmode].vm_ndim;

	sv.fv_x = ndim->fv_w / 2 + st.st_wioff.iv_x * st.st_winsp.iv_x;
	sv.fv_y = ndim->fv_h / 2 + st.st_wioff.iv_y * st.st_winsp.iv_y;
	sv.fv_z = ndim->fv_d / 2 + st.st_wioff.iv_z * st.st_winsp.iv_z;

	if ((q = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");
	gluQuadricDrawStyle(q, GLU_FILL);

	vec_set(&dim, 0.0f, 0.0f, WIV_SDEPTH);
	for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
		for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
			draw_cluster_pipe(q, &iv, &sv, &dim);

	vec_set(&dim, 0.0f, WIV_SHEIGHT, 0.0f);
	for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
		for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
			draw_cluster_pipe(q, &iv, &sv, &dim);

	vec_set(&dim, WIV_SWIDTH, 0.0f, 0.0f);
	for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
		for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
			draw_cluster_pipe(q, &iv, &sv, &dim);

	gluDeleteQuadric(q);
	glPopMatrix();
}

__inline void
draw_cluster_wired(const struct fvec *v)
{
	struct fvec *nv, dim, cldim, wrapv, spdim;
	struct ivec iv, adjv;
	struct node *n;

	/* Strict cluster dimensions. */
	cldim.fv_w = (widim.iv_w - 1) * st.st_winsp.iv_x;
	cldim.fv_h = (widim.iv_h - 1) * st.st_winsp.iv_y;
	cldim.fv_d = (widim.iv_d - 1) * st.st_winsp.iv_z;

	/* Spaced cluster dimensions. */
	spdim.fv_w = widim.iv_w * st.st_winsp.iv_x;
	spdim.fv_h = widim.iv_h * st.st_winsp.iv_y;
	spdim.fv_d = widim.iv_d * st.st_winsp.iv_z;

	if (st.st_opts & OP_PIPES &&
	    st.st_pipemode == PM_RT)
		draw_cluster_pipes(v);

	IVEC_FOREACH(&iv, &widim) {
		adjv.iv_x = negmod(iv.iv_x + st.st_wioff.iv_x, widim.iv_w);
		adjv.iv_y = negmod(iv.iv_y + st.st_wioff.iv_y, widim.iv_h);
		adjv.iv_z = negmod(iv.iv_z + st.st_wioff.iv_z, widim.iv_d);
		n= wimap[adjv.iv_x][adjv.iv_y][adjv.iv_z];
		if (n == NULL || !node_show(n))
			continue;

		wrapv.fv_x = floor((iv.iv_x + st.st_wioff.iv_x) / (double)widim.iv_w) * spdim.fv_w;
		wrapv.fv_y = floor((iv.iv_y + st.st_wioff.iv_y) / (double)widim.iv_h) * spdim.fv_h;
		wrapv.fv_z = floor((iv.iv_z + st.st_wioff.iv_z) / (double)widim.iv_d) * spdim.fv_d;

		nv = &n->n_swiv;
		nv->fv_x = n->n_wiv.iv_x * st.st_winsp.iv_x + v->fv_x + wrapv.fv_x;
		nv->fv_y = n->n_wiv.iv_y * st.st_winsp.iv_y + v->fv_y + wrapv.fv_y;
		nv->fv_z = n->n_wiv.iv_z * st.st_winsp.iv_z + v->fv_z + wrapv.fv_z;
		n->n_v = nv;
		draw_node(n, 0);
	}

	if (st.st_opts & OP_SKEL) {
		dim.fv_w = cldim.fv_w + NODEWIDTH  + 2 * SKEL_GAP;
		dim.fv_h = cldim.fv_h + NODEHEIGHT + 2 * SKEL_GAP;
		dim.fv_z = cldim.fv_d + NODEDEPTH  + 2 * SKEL_GAP;

		glPushMatrix();
		glTranslatef(-SKEL_GAP, -SKEL_GAP, -SKEL_GAP);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

		fill_light_blue.f_a = 0.4f;
		draw_box_outline(&dim, &fill_light_blue);
		fill_light_blue.f_a = 1.0f;

		glDisable(GL_BLEND);
		glPopMatrix();
	}

	if (st.st_opts & OP_PIPES &&
	    st.st_pipemode != PM_RT)
		draw_cluster_pipes(v);
}

__inline void
draw_wired_frame(struct fvec *vp, struct fvec *dimp, struct fill *fillp)
{
	struct fill fill = *fillp;
	struct fvec v;

	v = *vp;
	v.fv_x -= st.st_winsp.iv_x / 2.0f;
	v.fv_y -= st.st_winsp.iv_y / 2.0f;
	v.fv_z -= st.st_winsp.iv_z / 2.0f;

	glPushMatrix();
	glTranslatef(v.fv_x, v.fv_y, v.fv_z);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

	fill.f_a = 0.2;
	draw_box_filled(dimp, &fill);

	glDisable(GL_BLEND);
	glPopMatrix();
}

/* Snap to nearest grid */
__inline float
snap_to_grid(float n, float size, float clip)
{
	float adj;

	adj = fmod(n - clip, size);
//	while (adj < 0)
	if (adj < 0)
		adj += size;
	return (adj);
}

/*
 * Draw the cluster repeatedly till we reach the clipping plane.
 * Since we can see MIN(WIV_CLIP*) away, we must construct a 3D space
 * MIN(WIV_CLIP*)^3 large and draw the cluster multiple times inside.
 */
__inline void
draw_clusters_wired(void)
{
	int xnum, znum, col;
	struct fvec v, dim;
	float x, y, z;
	int opts;

	opts = st.st_opts;
	st.st_opts &= ~OP_NODEANIM;

	x = st.st_x - clip;
	y = st.st_y - clip;
	z = st.st_z - clip;

	x -= snap_to_grid(x, WIV_SWIDTH, 0.0);
	y -= snap_to_grid(y, WIV_SHEIGHT, 0.0);
	z -= snap_to_grid(z, WIV_SDEPTH, 0.0);

	wi_repstart.fv_x = x;
	wi_repstart.fv_y = y;
	wi_repstart.fv_z = z;

	dim.fv_w = WIV_SWIDTH - 0.01;
	dim.fv_h = WIV_SHEIGHT - 0.01;
	dim.fv_d = WIV_SDEPTH - 0.01;

	xnum = (st.st_x + clip - x + WIV_SWIDTH - 1) / WIV_SWIDTH;
	znum = (st.st_z + clip - z + WIV_SDEPTH - 1) / WIV_SDEPTH;

	col = 0;
	v.fv_z = v.fv_x = 0.0f; /* gcc */
	for (v.fv_y = y; v.fv_y < st.st_y + clip; v.fv_y += WIV_SHEIGHT) {
		for (v.fv_z = z; v.fv_z < st.st_z + clip; v.fv_z += WIV_SDEPTH) {
			for (v.fv_x = x; v.fv_x < st.st_x + clip; v.fv_x += WIV_SWIDTH) {
				draw_cluster_wired(&v);
				if (st.st_opts & OP_WIVMFRAME)
					draw_wired_frame(&v, &dim,
					    col ? &fill_grey : &fill_light_blue);
				col = !col;
			}
			if (xnum % 2 == 0)
				col = !col;
		}
		if (xnum % 2 == 0)
			col = !col;
		if ((((xnum % 2) ^ (znum % 2))))
			col = !col;
	}

	wi_repdim.fv_w = v.fv_x - wi_repstart.fv_x;
	wi_repdim.fv_h = v.fv_y - wi_repstart.fv_y;
	wi_repdim.fv_d = v.fv_z - wi_repstart.fv_z;

	st.st_opts = opts;
}

void
make_cluster(void)
{
	struct fvec fv;

	if (dl_cluster[wid])
		glDeleteLists(dl_cluster[wid], 1);
	dl_cluster[wid] = glGenLists(1);
	glNewList(dl_cluster[wid], GL_COMPILE);

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		draw_cluster_physical();
		break;
	case VM_WIRED:
		draw_clusters_wired();
		break;
	case VM_WIREDONE:
		vec_set(&fv, 0.0f, 0.0f, 0.0f);
		draw_cluster_wired(&fv);
		break;
	}

	glEndList();
}

void
make_selnodes(void)
{
	struct fvec pos, v;
	struct selnode *sn;
	struct fill *ofp;
	struct node *n;

	selnode_clean = 0;
	if (dl_selnodes[wid])
		glDeleteLists(dl_selnodes[wid], 1);
	if (SLIST_EMPTY(&selnodes)) {
		dl_selnodes[wid] = 0;
		return;
	}

	dl_selnodes[wid] = glGenLists(1);
	glNewList(dl_selnodes[wid], GL_COMPILE);

	vmodes[st.st_vmode].vm_ndim.fv_w += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_h += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_d += SELNODE_GAP * 2;

	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;
		ofp = n->n_fillp;
		n->n_fillp = &fill_selnode;

		switch (st.st_vmode) {
		case VM_WIRED:
			pos.fv_x = n->n_wiv.iv_x * st.st_winsp.iv_x - SELNODE_GAP;
			pos.fv_y = n->n_wiv.iv_y * st.st_winsp.iv_y - SELNODE_GAP;
			pos.fv_z = n->n_wiv.iv_z * st.st_winsp.iv_z - SELNODE_GAP;
			WIREP_FOREACH(&v) {
				glPushMatrix();
				glTranslatef(
				    pos.fv_x + v.fv_x,
				    pos.fv_y + v.fv_y,
				    pos.fv_z + v.fv_z);
				draw_node(n, NDF_DONTPUSH | NDF_NOOPTS);
				glPopMatrix();

				if (st.st_opts & OP_SELPIPES &&
				    (st.st_opts & OP_PIPES) == 0)
					draw_node_pipes(n);
			}
			break;
		default:
			n->n_v->fv_x -= SELNODE_GAP;
			n->n_v->fv_y -= SELNODE_GAP;
			n->n_v->fv_z -= SELNODE_GAP;

			glPushMatrix();
			glTranslatef(n->n_v->fv_x, n->n_v->fv_y, n->n_v->fv_z);
			draw_node(n, NDF_DONTPUSH | NDF_NOOPTS);
			glPopMatrix();

			if (st.st_vmode != VM_PHYSICAL &&
			    st.st_opts & OP_SELPIPES &&
			    (st.st_opts & OP_PIPES) == 0)
				draw_node_pipes(n);

			n->n_v->fv_x += SELNODE_GAP;
			n->n_v->fv_y += SELNODE_GAP;
			n->n_v->fv_z += SELNODE_GAP;
			break;
		}
		n->n_fillp = ofp;
	}
	vmodes[st.st_vmode].vm_ndim.fv_w -= SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_h -= SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_d -= SELNODE_GAP * 2;

	glEndList();
}
