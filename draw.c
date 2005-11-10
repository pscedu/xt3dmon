/* $Id$ */

#include "compat.h"

#include <math.h>

#include "mon.h"
#include "cdefs.h"

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

struct fvec wivstart, wivdim;
float clip;

struct fill fill_black		= { 0.0f, 0.0f, 0.0f, 1.0f, 0, 0 };
struct fill fill_grey		= { 0.2f, 0.2f, 0.2f, 1.0f, 0, 0 };
struct fill fill_light_blue	= { 0.2f, 0.4f, 0.6f, 1.0f, 0, 0 };
struct fill fill_yellow		= { 1.0f, 1.0f, 0.0f, 1.0f, 0, 0 };
struct fill fill_selnode	= { 0.2f, 0.4f, 0.6f, 1.0f, 0, 0 };

struct fvec fvzero = { 0.0f, 0.0f, 0.0f };

#define NEARCLIP (1.0)

__inline void
wired_update(void)
{
	if (st.st_x + clip > wivstart.fv_x + wivdim.fv_w ||
	    st.st_x - clip < wivstart.fv_x ||
	    st.st_y + clip > wivstart.fv_y + wivdim.fv_h ||
	    st.st_y - clip < wivstart.fv_y ||
	    st.st_z + clip > wivstart.fv_z + wivdim.fv_d ||
	    st.st_z - clip < wivstart.fv_z) {
		status_add("Rebuild triggered\n");
		st.st_rf |= RF_CLUSTER;
	}
}

__inline void
draw_scene(int wid)
{
	if (select_dl[wid])
		glCallList(select_dl[wid]);
	glCallList(cluster_dl[wid]);
	if (st.st_opts & OP_GROUND)
		glCallList(ground_dl[wid]);
	if (!TAILQ_EMPTY(&panels))
		draw_panels(wid);
}

#define FOCAL_POINT (2.0f) /* distance from cam to center of 3d focus */
#define FOCAL_LENGTH (5.0f) /* length of 3d focus */

void
drawh_stereo(void)
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
		glutSetWindow(window_ids[WINID_RIGHT]);
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
	draw_scene(WINID_RIGHT);

	if (stereo_mode == STM_PASV) {
		glClearColor(0.2, 0.2, 0.2, 1.0);
		/* XXX: capture frame */
		if (st.st_opts & OP_CAPTURE)
			capture_frame(capture_mode);
		else if (st.st_opts & OP_DISPLAY)
			glutSwapBuffers();
	}

	/* Draw left buffer. */
	switch (stereo_mode) {
	case STM_ACT:
		glDrawBuffer(GL_BACK_LEFT);
		break;
	case STM_PASV:
		glutSetWindow(window_ids[WINID_LEFT]);
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
	draw_scene(WINID_LEFT);

	glClearColor(0.2, 0.2, 0.2, 1.0);
	if (st.st_opts & OP_CAPTURE)
		capture_frame(capture_mode);
	else if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();

	/* Restore camera position. */
	st.st_x += stereo_fv.fv_x;
	st.st_y += stereo_fv.fv_y;
	st.st_z += stereo_fv.fv_z;
}

void
drawh_default(void)
{
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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (cam_dirty) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(FOVY, ASPECT, NEARCLIP, clip);
		glMatrixMode(GL_MODELVIEW);
		cam_look();
	}
	draw_scene(WINID_DEF);
	cam_dirty = 0;

	glClearColor(0.0, 0.0, 0.0, 1.0);
	if (st.st_opts & OP_CAPTURE)
		capture_frame(capture_mode);
	else if (st.st_opts & OP_DISPLAY)
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
	int nid;
	int i = 0;

	/*
	 * Parse the node id for use with
	 * NODE0123456789 (so 4 letter gap before 0)
	 */
	nid = n->n_nid;
	while (nid >= 0 && i < MAX_CHARS) {
		list[MAX_CHARS-i-1] = 4 + nid % 10;
		nid /= 10;
		i++;
	}

	while (i < MAX_CHARS)
		list[i++] = -1;

	glEnable(GL_TEXTURE_2D);

	/* Get a distinct contrast color */
	c = *n->n_fillp;
	rgb_contrast(&c);
	glColor4f(c.f_r, c.f_g, c.f_b, c.f_a);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, font_id);

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
draw_node_pipes(struct fvec *dim)
{
	float w = dim->fv_w, h = dim->fv_h, d = dim->fv_d;

	/* Antialiasing */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(8.0);
	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 1.0f); 			/* x - blue */
	glVertex3f(w - st.st_winsp.iv_x, h/2, d/2);
	glVertex3f(st.st_winsp.iv_x, h/2, d/2);

	glColor3f(1.0f, 0.0f, 0.0f);			/* y - red */
	glVertex3f(w/2, h - st.st_winsp.iv_y, d/2);
	glVertex3f(w/2, st.st_winsp.iv_y, d/2);

	glColor3f(0.0f, 1.0f, 0.0f);			/* z - green */
	glVertex3f(w/2, h/2, d - st.st_winsp.iv_z);
	glVertex3f(w/2, h/2, st.st_winsp.iv_z);
	glEnd();

	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);

	glColor3f(0.0, 0.0, 0.0);
	glPointSize(20.0);
	glBegin(GL_POINTS);
	glVertex3f(w/2.0, h/2.0, d/2.0);
	glEnd();

	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

__inline void
draw_node(struct node *n, int flags)
{
	struct fill *fp, *fill_wireframe;
	struct fvec *vp, *dimp;

	GLenum param = GL_REPLACE;

	if (!node_show(n))
		return;

	fp = n->n_fillp;
	vp = n->n_v;
	dimp = &vmodes[st.st_vmode].vm_ndim;

	if ((flags & NDF_DONTPUSH) == 0) {
		glPushMatrix();
		/*
		 * We assume that the stack we are in otherwise
		 * has done the translation itself.
		 */
		glTranslatef(vp->fv_x, vp->fv_y, vp->fv_z);
	}

	if (fp->f_a != 1.0f) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		param = GL_BLEND;
	}

	if (n->n_flags & NF_SKEL)
		fill_wireframe = &fill_yellow;
	else {
		fill_wireframe = &fill_black;

		if (st.st_opts & OP_TEX)
			draw_box_tex(dimp, fp, param);
		else
			draw_box_filled(dimp, fp);
	}

	if (fp->f_a != 1.0f)
		glDisable(GL_BLEND);

	if (st.st_opts & OP_WIREFRAME)
		draw_box_outline(dimp, fill_wireframe);
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
make_ground(int wid)
{
	struct fvec fv, fdim;
	struct fill fill;

	if (ground_dl[wid])
		glDeleteLists(ground_dl[wid], 1);
	ground_dl[wid] = glGenLists(1);
	glNewList(ground_dl[wid], GL_COMPILE);

	/* Ground */
	fill.f_r = 0.3f;
	fill.f_g = 0.3f;
	fill.f_b = 0.3f;
	fill.f_a = 0.1f;
	switch (st.st_vmode) {
	case VM_WIREDONE:
		fv.fv_x = -st.st_winsp.iv_x / 2.0f;
		fv.fv_y = -0.2f;
		fv.fv_z = -st.st_winsp.iv_z / 2.0f;

		fdim.fv_w = WIV_SWIDTH - fv.fv_x;
		fdim.fv_y = -fv.fv_y / 2.0f;
		fdim.fv_d = WIV_SDEPTH - fv.fv_z;

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

	/* Antialiasing */
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

	glEndList();
}

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
#if 0
				if (st.st_opts & OP_SKEL && cg) {
					glPushMatrix();
					glTranslatef(v.fv_x - SKEL_GAP,
					    v.fv_y - CAGESPACE / 2.0f,
					    v.fv_z - SKEL_GAP);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
					glBegin(GL_LINE_LOOP);
					glColor4f(fill_light_blue.f_r,
					    fill_light_blue.f_g,
					    fill_light_blue.f_b, 0.4f);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(CABWIDTH + 2 * SKEL_GAP, 0.0f, 0.0f);
					glVertex3f(CABWIDTH + 2 * SKEL_GAP, 0.0f,
					    MODDEPTH + 2 * SKEL_GAP);
					glVertex3f(0.0f, 0.0f,
					    MODDEPTH + 2 * SKEL_GAP);
					glEnd();
					glDisable(GL_BLEND);
					glPopMatrix();
				}
#endif
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
	struct fvec *dimp;

	dimp = &vmodes[st.st_vmode].vm_ndim;
	spx = st.st_winsp.iv_x;
	spy = st.st_winsp.iv_y;
	spz = st.st_winsp.iv_z;
	sx = dimp->fv_w / 2;
	sy = dimp->fv_h / 2;
	sz = dimp->fv_d / 2;

	glPushMatrix();
	glTranslatef(v->fv_x, v->fv_y, v->fv_z);

	/* Antialiasing */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(8.0);

	glBegin(GL_LINES);
	glColor3f(0.0f, 1.0f, 0.0f);
	for (x = sx; x < WIV_SWIDTH; x += spx)			/* z - green */
		for (y = sy; y < WIV_SHEIGHT; y += spy) {
			glVertex3f(x, y, -dimp->fv_z / 2.0f);
			glVertex3f(x, y, WIV_SDEPTH);
		}

	glColor3f(1.0f, 0.0f, 0.0f);
	for (z = sz; z < WIV_SDEPTH; z += spz)			/* y - red */
		for (x = sx; x < WIV_SWIDTH; x += spx) {
			glVertex3f(x, -dimp->fv_y / 2.0f, z);
			glVertex3f(x, WIV_SHEIGHT, z);
		}

	glColor3f(0.0f, 0.0f, 1.0f);
	for (y = sy; y < WIV_SHEIGHT; y += spy)
		for (z = sz; z < WIV_SDEPTH; z += spz) {	/* x - blue */
			glVertex3f(-dimp->fv_x / 2.0f, y, z);
			glVertex3f(WIV_SWIDTH, y, z);
		}
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
	glEnd();
	glPopMatrix();
}

__inline void
draw_cluster_wired(struct fvec *v)
{
	int r, cb, cg, m, n;
	struct node *node;
	struct fvec *nv;

	if (st.st_opts & OP_PIPES)
		draw_cluster_pipes(v);
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						nv = &node->n_swiv;
						nv->fv_x = node->n_wiv.iv_x * st.st_winsp.iv_x + v->fv_x;
						nv->fv_y = node->n_wiv.iv_y * st.st_winsp.iv_y + v->fv_y;
						nv->fv_z = node->n_wiv.iv_z * st.st_winsp.iv_z + v->fv_z;
						node->n_v = nv;
						draw_node(node, 0);
					}
	if (st.st_opts & OP_SKEL) {
		struct fvec dim;

		dim.fv_w = ((WIDIM_WIDTH  - 1) * st.st_winsp.iv_x) + NODEWIDTH  + 2 * SKEL_GAP;
		dim.fv_h = ((WIDIM_HEIGHT - 1) * st.st_winsp.iv_x) + NODEHEIGHT + 2 * SKEL_GAP;
		dim.fv_z = ((WIDIM_DEPTH  - 1) * st.st_winsp.iv_x) + NODEDEPTH  + 2 * SKEL_GAP;

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
	if(adj < 0)
		adj += size;

	return adj;
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
	float x, y, z;
	struct fvec v, dim;

//	clip = MIN3(WIV_CLIPX, WIV_CLIPY, WIV_CLIPZ);

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
}

void
make_cluster(int wid)
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
make_select(int wid)
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
						glPushMatrix();
						glTranslatef(
						    pos.fv_x + v.fv_x,
						    pos.fv_y + v.fv_y,
						    pos.fv_z + v.fv_z);
						if (st.st_opts & OP_SELPIPES &&
						    (st.st_opts & OP_PIPES) == 0)
							draw_node_pipes(&vmodes[st.st_vmode].vm_ndim);

						draw_node(n, NDF_DONTPUSH | NDF_NOOPTS);
						glPopMatrix();
					}
			break;
		default:
			n->n_v->fv_x -= SELNODE_GAP;
			n->n_v->fv_y -= SELNODE_GAP;
			n->n_v->fv_z -= SELNODE_GAP;

			glPushMatrix();
			glTranslatef(n->n_v->fv_x, n->n_v->fv_y, n->n_v->fv_z);
			if (st.st_vmode != VM_PHYSICAL &&
			    st.st_opts & OP_SELPIPES && (st.st_opts & OP_PIPES) == 0)
				draw_node_pipes(&vmodes[st.st_vmode].vm_ndim);

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
