/* $Id$ */

#include "compat.h"

#include <math.h>

#include "cdefs.h"
#include "mon.h"
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

#define NEARCLIP (1.0)
#define FOCAL_POINT (2.0f) /* distance from cam to center of 3d focus */
#define FOCAL_LENGTH (5.0f) /* length of 3d focus */

struct fvec wivstart, wivdim;
struct ivec wioff;
float clip;

struct fill fill_bg		= FILL_INIT(0.1f, 0.2f, 0.3f);
struct fill fill_black		= FILL_INIT(0.0f, 0.0f, 0.0f);
struct fill fill_white		= FILL_INIT(1.0f, 1.0f, 1.0f);
struct fill fill_grey		= FILL_INIT(0.2f, 0.2f, 0.2f);
struct fill fill_light_blue	= FILL_INIT(0.2f, 0.4f, 0.6f);
struct fill fill_yellow		= FILL_INIT(1.0f, 1.0f, 0.0f);
struct fill fill_selnode	= FILL_INIT(0.2f, 0.4f, 0.6f);
struct fill fill_font		= FILL_INIT(0.0f, 0.0f, 0.0f);
struct fill fill_borg		= FILL_INIT(0.0f, 0.0f, 0.0f);
struct fill fill_nodata		= FILL_INITF(1.0f, 1.0f, 0.0f, FF_SKEL);
struct fill fill_matrix		= FILL_INITF(0.0f, 1.0f, 0.0f, FF_SKEL);
struct fill fill_matrix_reloaded= FILL_INITA(0.0f, 1.0f, 0.0f, 0.3);

struct fvec fvzero = { 0.0f, 0.0f, 0.0f };

__inline void
wired_update(void)
{
	if (st.st_x + clip > wivstart.fv_x + wivdim.fv_w ||
	    st.st_x - clip < wivstart.fv_x ||
	    st.st_y + clip > wivstart.fv_y + wivdim.fv_h ||
	    st.st_y - clip < wivstart.fv_y ||
	    st.st_z + clip > wivstart.fv_z + wivdim.fv_d ||
	    st.st_z - clip < wivstart.fv_z) {
//		status_add("Rebuild triggered\n");
		st.st_rf |= RF_CLUSTER;
	}
}

void
draw_compass(void)
{
	struct fvec fv, upadj, normv;
	GLUquadric *q;

	cam_getspecvec(&fv, 150, win_height - 150);
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
	if (select_dl[wid])
		glCallList(select_dl[wid]);
	glCallList(cluster_dl[wid]);
	if (st.st_opts & OP_GROUND)
		glCallList(ground_dl[wid]);
	if (!TAILQ_EMPTY(&panels))
		draw_panels(wid);
//draw_compass();
//job_drawlabels();
}

void
gl_displayh_stereo(void)
{
	static double ratio, radians, wd2, ndfl, eyesep;
	static float left, right, top, bottom;
	static struct fvec stereo_fv;

	if (flyby_mode)
		flyby_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	if (st.st_rf) {
		rebuild(st.st_rf);
		st.st_rf = 0;
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
	if (flyby_mode)
		flyby_update();
	if (st.st_opts & OP_TWEEN)
		tween_update();
	if (st.st_vmode == VM_WIRED)
		wired_update();
	if (st.st_rf) {
		int rf;

		rf = st.st_rf;
		st.st_rf = 0;
		rebuild(rf);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (cam_dirty) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
		glMatrixMode(GL_MODELVIEW);
		cam_look();
	}
	draw_scene();
	cam_dirty = 0;

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
		rgb_contrast(&c);
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

	glColor3f(0.0f, 1.0f, 0.0f);					/* z - green */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + ndim->fv_w / 2.0,
	    np->n_v->fv_y + ndim->fv_h / 2.0,
	    np->n_v->fv_z + ndim->fv_d / 2.0 - st.st_winsp.iv_d);
	gluCylinder(q, 0.1, 0.1, 2.0 * st.st_winsp.iv_d, 3, 1);
	glPopMatrix();

	glColor3f(1.0f, 0.0f, 0.0f);					/* y - red */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + ndim->fv_w / 2.0,
	    np->n_v->fv_y + ndim->fv_h / 2.0 - st.st_winsp.iv_h,
	    np->n_v->fv_z + ndim->fv_d / 2.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(q, 0.1, 0.1, 2.0 * st.st_winsp.iv_h, 3, 1);
	glPopMatrix();

	glColor3f(0.0f, 0.0f, 1.0f);					/* x - blue */
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

	if (ground_dl[wid])
		glDeleteLists(ground_dl[wid], 1);
	ground_dl[wid] = glGenLists(1);
	glNewList(ground_dl[wid], GL_COMPILE);

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
		fv.fv_x = st.st_winsp.iv_x * (-0.5f + wioff.iv_x);
		fv.fv_y = -0.2f + wioff.iv_y * st.st_winsp.iv_y;
		fv.fv_z = st.st_winsp.iv_z * (-0.5f + wioff.iv_z);

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

__inline void
draw_cluster_pipes(struct fvec *v)
{
	float x, y, z, spx, spy, spz;
	float sx, sy, sz;
	struct fvec cldim, *ndim;
	GLUquadric *q;

	glPushMatrix();
	glTranslatef(v->fv_x, v->fv_y, v->fv_z);

	ndim = &vmodes[st.st_vmode].vm_ndim;

	spx = st.st_winsp.iv_x;
	spy = st.st_winsp.iv_y;
	spz = st.st_winsp.iv_z;

	sx = ndim->fv_w / 2 + wioff.iv_x * spx;
	sy = ndim->fv_h / 2 + wioff.iv_y * spy;
	sz = ndim->fv_d / 2 + wioff.iv_z * spz;

	cldim.fv_w = WIV_SWIDTH;
	cldim.fv_h = WIV_SHEIGHT;
	cldim.fv_d = WIV_SDEPTH;

	if ((q = gluNewQuadric()) == NULL)
		err(1, "gluNewQuadric");
	gluQuadricDrawStyle(q, GLU_FILL);

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	glColor3f(0.0f, 1.0f, 0.0f);					/* z - green */
	for (x = sx; x - sx < cldim.fv_w; x += spx)
		for (y = sy; y - sy < cldim.fv_h; y += spy) {
			glPushMatrix();
			glTranslatef(x, y, sz - spz);
			gluCylinder(q, 0.1, 0.1, cldim.fv_d + spz, 3, 1);
			glPopMatrix();
		}

	glColor3f(1.0f, 0.0f, 0.0f);					/* y - red */
	for (z = sz; z - sz < cldim.fv_d; z += spz)
		for (x = sx; x - sx < cldim.fv_w; x += spx) {
			glPushMatrix();
			glTranslatef(x, sy - spy, z);
			glRotatef(-90.0, 1.0, 0.0, 0.0);
			gluCylinder(q, 0.1, 0.1, cldim.fv_h + spy, 3, 1);
			glPopMatrix();
		}

	glColor3f(0.0f, 0.0f, 1.0f);					/* x - blue */
	for (y = sy; y - sy < cldim.fv_h; y += spy)
		for (z = sz; z - sz < cldim.fv_d; z += spz) {
			glPushMatrix();
			glTranslatef(sx - spx, y, z);
			glRotatef(90.0, 0.0, 1.0, 0.0);
			gluCylinder(q, 0.1, 0.1, cldim.fv_w + spx, 3, 1);
			glPopMatrix();
		}
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_BLEND);
	gluDeleteQuadric(q);

	glPopMatrix();
}

__inline void
draw_cluster_wired(struct fvec *v)
{
	struct fvec *nv, dim, cldim, wrapv;
	struct ivec iv, adjv;
	struct node *node;

	cldim.fv_w = (WIDIM_WIDTH  - 1) * st.st_winsp.iv_x;
        cldim.fv_h = (WIDIM_HEIGHT - 1) * st.st_winsp.iv_y;
        cldim.fv_d = (WIDIM_DEPTH  - 1) * st.st_winsp.iv_z;

	if (st.st_opts & OP_PIPES)
		draw_cluster_pipes(v);

	for (iv.iv_x = 0; iv.iv_x < WIDIM_WIDTH; iv.iv_x++)
		for (iv.iv_y = 0; iv.iv_y < WIDIM_HEIGHT; iv.iv_y++)
			for (iv.iv_z = 0; iv.iv_z < WIDIM_DEPTH; iv.iv_z++) {
				adjv.iv_x = negmod(iv.iv_x + wioff.iv_x, WIDIM_WIDTH);
				adjv.iv_y = negmod(iv.iv_y + wioff.iv_y, WIDIM_HEIGHT);
				adjv.iv_z = negmod(iv.iv_z + wioff.iv_z, WIDIM_DEPTH);
				node = wimap[adjv.iv_x][adjv.iv_y][adjv.iv_z];
				if (node == NULL || !node_show(node))
					continue;

				wrapv.fv_x = floor((iv.iv_x + wioff.iv_x) / (double)WIDIM_WIDTH)  * (WIDIM_WIDTH  * st.st_winsp.iv_x);
				wrapv.fv_y = floor((iv.iv_y + wioff.iv_y) / (double)WIDIM_HEIGHT) * (WIDIM_HEIGHT * st.st_winsp.iv_y);
				wrapv.fv_z = floor((iv.iv_z + wioff.iv_z) / (double)WIDIM_DEPTH)  * (WIDIM_DEPTH  * st.st_winsp.iv_z);

				nv = &node->n_swiv;
				nv->fv_x = node->n_wiv.iv_x * st.st_winsp.iv_x + v->fv_x + wrapv.fv_x;
				nv->fv_y = node->n_wiv.iv_y * st.st_winsp.iv_y + v->fv_y + wrapv.fv_y;
				nv->fv_z = node->n_wiv.iv_z * st.st_winsp.iv_z + v->fv_z + wrapv.fv_z;
				node->n_v = nv;
				draw_node(node, 0);
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

	wivstart.fv_x = x;
	wivstart.fv_y = y;
	wivstart.fv_z = z;

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

	wivdim.fv_w = v.fv_x - wivstart.fv_x;
	wivdim.fv_h = v.fv_y - wivstart.fv_y;
	wivdim.fv_d = v.fv_z - wivstart.fv_z;

	st.st_opts = opts;
}

void
make_cluster(void)
{
	if (cluster_dl[wid])
		glDeleteLists(cluster_dl[wid], 1);
	cluster_dl[wid] = glGenLists(1);
	glNewList(cluster_dl[wid], GL_COMPILE);

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		draw_cluster_physical();
		break;
	case VM_WIRED:
		draw_clusters_wired();
		break;
	case VM_WIREDONE: {
		struct fvec v = { 0.0f, 0.0f, 0.0f };

		draw_cluster_wired(&v);
		break;
	    }
	}

	glEndList();
}

void
make_select(void)
{
	struct fvec pos, v;
	struct selnode *sn;
	struct fill *ofp;
	struct node *n;

	selnode_clean = 0;
	if (select_dl[wid])
		glDeleteLists(select_dl[wid], 1);
	if (SLIST_EMPTY(&selnodes)) {
		select_dl[wid] = 0;
		return;
	}

	select_dl[wid] = glGenLists(1);
	glNewList(select_dl[wid], GL_COMPILE);

	vmodes[st.st_vmode].vm_ndim.fv_w += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_h += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_d += SELNODE_GAP * 2;

	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;
		ofp = n->n_fillp;
		n->n_fillp = &fill_selnode;

		switch (st.st_vmode) {
		case VM_WIRED:
			pos.fv_x = n->n_wiv.iv_x * st.st_winsp.iv_x + wivstart.fv_x - SELNODE_GAP;
			pos.fv_y = n->n_wiv.iv_y * st.st_winsp.iv_y + wivstart.fv_y - SELNODE_GAP;
			pos.fv_z = n->n_wiv.iv_z * st.st_winsp.iv_z + wivstart.fv_z - SELNODE_GAP;
			for (v.fv_x = 0.0f; v.fv_x < wivdim.fv_w; v.fv_x += WIV_SWIDTH)
				for (v.fv_y = 0.0f; v.fv_y < wivdim.fv_h; v.fv_y += WIV_SHEIGHT)
					for (v.fv_z = 0.0f; v.fv_z < wivdim.fv_d; v.fv_z += WIV_SDEPTH) {
						if (st.st_opts & OP_SELPIPES &&
						    (st.st_opts & OP_PIPES) == 0)
							draw_node_pipes(n);

						glPushMatrix();
						glTranslatef(
						    pos.fv_x + v.fv_x,
						    pos.fv_y + v.fv_y,
						    pos.fv_z + v.fv_z);
						draw_node(n, NDF_DONTPUSH | NDF_NOOPTS);
						glPopMatrix();
					}
			break;
		default:
			n->n_v->fv_x -= SELNODE_GAP;
			n->n_v->fv_y -= SELNODE_GAP;
			n->n_v->fv_z -= SELNODE_GAP;

			if (st.st_vmode != VM_PHYSICAL &&
			    st.st_opts & OP_SELPIPES && (st.st_opts & OP_PIPES) == 0)
				draw_node_pipes(n);

			glPushMatrix();
			glTranslatef(n->n_v->fv_x, n->n_v->fv_y, n->n_v->fv_z);
			draw_node(n, NDF_DONTPUSH | NDF_NOOPTS);
			glPopMatrix();

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
