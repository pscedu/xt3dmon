/* $Id$ */

#include <sys/time.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <math.h>
#include <stdio.h>

#include "mon.h"
#include "cdefs.h"

#define TWEEN_MAX_POS	(2.0f)
#define TWEEN_MAX_LOOK	(0.04f)

struct vec wivstart, wivdim;
float clip;

struct fill fill_black = { 0.0f, 0.0f, 0.0f, 1.0f, 0, 0 };
struct fill fill_grey  = { 0.2f, 0.2f, 0.2f, 1.0f, 0, 0 };
struct fill fill_light_blue = { 0.2f, 0.4f, 0.6f, 1.0f, 0, 0 };

void
draw(void)
{
	if (flyby_mode)
		flyby_update();

	if (render_mode == RM_SELECT)
		sel_record_begin();

	if (st.st_opts & OP_TWEEN) {
		/* XXX: benchmark to test static. */
		static struct vec want;
		static struct vec want_l;
		static struct vec sc, sc_l;
		float scale, scale_l;

		memset(&want, 0, sizeof(want));
		memset(&want_l, 0, sizeof(want_l));
		sc.v_x = sc.v_y = sc.v_z = 1.0;
		sc_l.v_x = sc_l.v_y = sc_l.v_z = 1.0;

		tween_probe(&st.st_x, tx, TWEEN_MAX_POS, &sc.v_x, &want.v_w);
		tween_probe(&st.st_y, ty, TWEEN_MAX_POS, &sc.v_y, &want.v_h);
		tween_probe(&st.st_z, tz, TWEEN_MAX_POS, &sc.v_z, &want.v_d);
		tween_probe(&st.st_lx, tlx, TWEEN_MAX_LOOK, &sc_l.v_x, &want_l.v_w);
		tween_probe(&st.st_ly, tly, TWEEN_MAX_LOOK, &sc_l.v_y, &want_l.v_h);
		tween_probe(&st.st_lz, tlz, TWEEN_MAX_LOOK, &sc_l.v_z, &want_l.v_d);

		scale = MIN3(sc.v_x, sc.v_y, sc.v_z);
		scale_l = MIN3(sc_l.v_x, sc_l.v_y, sc_l.v_z);

		tween_recalc(&st.st_x, tx, scale, want.v_w);
		tween_recalc(&st.st_y, ty, scale, want.v_h);
		tween_recalc(&st.st_z, tz, scale, want.v_d);
		tween_recalc(&st.st_lx, tlx, scale_l, want_l.v_w);
		tween_recalc(&st.st_ly, tly, scale_l, want_l.v_h);
		tween_recalc(&st.st_lz, tlz, scale_l, want_l.v_d);

		if (want.v_w || want.v_h || want.v_d ||
		    want_l.v_w || want_l.v_h || want_l.v_d)
			cam_update();
	}

	if (st.st_vmode == VM_WIRED)
		if (st.st_x + clip > wivstart.v_x + wivdim.v_w ||
		    st.st_x - clip < wivstart.v_x ||
		    st.st_y + clip > wivstart.v_y + wivdim.v_h ||
		    st.st_y - clip < wivstart.v_y ||
		    st.st_z + clip > wivstart.v_z + wivdim.v_d ||
		    st.st_z - clip < wivstart.v_z)
			st.st_rf |= RF_CLUSTER;

	if (st.st_rf) {
		rebuild(st.st_rf);
		st.st_rf = 0;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (st.st_opts & OP_GROUND)
		glCallList(ground_dl);
	if (select_dl)
		glCallList(select_dl);
	glCallList(cluster_dl);
	if (!TAILQ_EMPTY(&panels))
		draw_panels();

	glClearColor(0.0, 0.0, 0.2, 1.0);
	if (st.st_opts & OP_CAPTURE)
		capture_fb(capture_mode);
	if (render_mode == RM_SELECT)
		sel_record_end();
	else if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();
}

/* Render a char from the font texture */
__inline void
draw_char(int ch, float x, float y, float z)
{
	if (ch < 0)
		return;

	glTexCoord2f(FONT_TEXCOORD_S*ch, 0.0);
	glVertex3f(x, y, z);

	glTexCoord2f(FONT_TEXCOORD_S*ch, FONT_TEXCOORD_T);
	glVertex3f(x, y+FONT_DISPLACE_H, z);

	glTexCoord2f(FONT_TEXCOORD_S*(ch+1), FONT_TEXCOORD_T);
	glVertex3f(x, y+FONT_DISPLACE_H, z+FONT_DISPLACE_W);

	glTexCoord2f(FONT_TEXCOORD_S*(ch+1), 0.0);
	glVertex3f(x, y, z+FONT_DISPLACE_W);
}

__inline void
draw_node_label(struct node *n)
{
	float list[MAX_CHARS];
	struct fill c;
	int nid;
	int i, j;

	/* texture position for 'NODE' */
	list[0] = 0;
	list[1] = 1;
	list[2] = 2;
	list[3] = 3;
	i = 4;

	/*
	** Parse the node id for use with
	** NODE0123456789 (so 4 letter gap before 0)
	*/
	nid = n->n_nid;
	while(nid >= 0 && i < MAX_CHARS) {
		list[MAX_CHARS-i+3] = 4 + nid % 10;
		nid /= 10;
		i++;
	}

	while(i < 8)
		list[i++] = -1;

	glEnable(GL_TEXTURE_2D);

	/* Get a distinct contrast color */
	c = *n->n_fillp;
	rgb_contrast(&c);
	glColor4f(c.f_r, c.f_g, c.f_b, c.f_a);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, font_id);

	glBegin(GL_QUADS);

	for(i = 0, j = 0; i < 8; i++, j++) {
		/* Place a space between 'NODE' and id */
		if (j == 4)
			j++;
		draw_char(list[i], -0.001, NODEDEPTH/2.0,
		    FONT_Z_OFFSET+FONT_DISPLACE_W*(float)(j));
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
draw_node_pipes(struct vec *dim)
{
	float w = dim->v_w, h = dim->v_h, d = dim->v_d;

	/* Antialiasing */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(8.0);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f); 			/* x */
	glVertex3f(w - st.st_winspx, h/2, d/2);
	glVertex3f(st.st_winspx, h/2, d/2);

	glColor3f(0.0f, 1.0f, 0.0f);			/* y */
	glVertex3f(w/2, h - st.st_winspy, d/2);
	glVertex3f(w/2, st.st_winspy, d/2);

	glColor3f(0.0f, 0.0f, 1.0f);			/* z */
	glVertex3f(w/2, h/2, d - st.st_winspz);
	glVertex3f(w/2, h/2, st.st_winspz);
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
	struct vec *vp, *dimp;
	struct fill *fp;

	if (n->n_hide)
		return;

	fp = n->n_fillp;
	vp = n->n_v;
	dimp = &vmodes[st.st_vmode].vm_ndim;

	if ((flags & NDF_DONTPUSH) == 0) {
		glPushMatrix();
		glPushName(n->n_nid);
		glTranslatef(vp->v_x, vp->v_y, vp->v_z);
	}

	if (flags & NDF_NOOPTS)
		draw_box_filled(dimp, fp);
	else if (st.st_opts & OP_TEX)
		draw_box_tex(dimp, fp);
	else {
		if (st.st_opts & OP_BLEND) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		}
		draw_box_filled(dimp, fp);
		if (st.st_opts & OP_BLEND)
			glDisable(GL_BLEND);
	}
	if (st.st_opts & OP_WIREFRAME)
		draw_box_outline(dimp, &fill_black);
	if (st.st_opts & OP_NLABELS)
		draw_node_label(n);

	if ((flags & NDF_DONTPUSH) == 0) {
		glPopName();
		glPopMatrix();
	}
}

__inline void
draw_mod(struct vec *vp, struct vec *dim, struct fill *fillp)
{
	struct vec v;

	v = *vp;
	v.v_x -= 0.01;
	v.v_y -= 0.01;
	v.v_z -= 0.01;
	glPushMatrix();
	glTranslatef(v.v_x, v.v_y, v.v_z);

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
	if (ground_dl)
		glDeleteLists(ground_dl, 1);
	ground_dl = glGenLists(1);
	glNewList(ground_dl, GL_COMPILE);

	glBegin(GL_QUADS);

	/* Ground */
	glColor3f(0.4f, 0.4f, 0.4f);
	switch (st.st_vmode) {
	case VM_WIREDONE:
		glVertex3f( -st.st_winspx / 2.0f, -0.1f, -st.st_winspz / 2.0f);
		glVertex3f( -st.st_winspx / 2.0f, -0.1f, WI_DEPTH);
		glVertex3f(WI_WIDTH, -0.1f, WI_DEPTH);
		glVertex3f(WI_WIDTH, -0.1f, -st.st_winspz / 2.0f);
		break;
	case VM_PHYSICAL:
		glVertex3f( -5.0f, -0.1f, -5.0f);
		glVertex3f( -5.0f, -0.1f, 2*ROWDEPTH + ROWSPACE + 5.0f);
		glVertex3f(ROWWIDTH + 5.0f, -0.1f, 2*ROWDEPTH + ROWSPACE + 5.0f);
		glVertex3f(ROWWIDTH + 5.0f, -0.1f, -5.0f);
		break;
	}
	glEnd();

	glLineWidth(5.0);
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
	glEndList();
}

__inline void
draw_cluster_physical(void)
{
	int r, cb, cg, m, s, n0, n;
	struct node *node;
	struct vec mdim;
	struct fill mf;
	struct vec v;

	mdim.v_w = MODWIDTH + 0.02;
	mdim.v_h = MODHEIGHT + 0.02;
	mdim.v_d = MODDEPTH + 0.02;

	mf.f_r = 1.00;
	mf.f_g = 1.00;
	mf.f_b = 1.00;
	mf.f_a = 0.30;

	v.v_x = v.v_y = v.v_z = NODESPACE;
	for (r = 0; r < NROWS; r++, v.v_z += ROWDEPTH + ROWSPACE) {
		for (cb = 0; cb < NCABS; cb++, v.v_x += CABWIDTH + CABSPACE) {
			for (cg = 0; cg < NCAGES; cg++, v.v_y += CAGEHEIGHT + CAGESPACE) {
				for (m = 0; m < NMODS; m++, v.v_x += MODWIDTH + MODSPACE) {
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_v = &node->n_physv;
						node->n_physv = v;

						s = n / (NNODES / 2);
						n0 = (n & 1) ^ ((n & 2) >> 1);

						node->n_physv.v_y += s * (NODESPACE + NODEHEIGHT);
						node->n_physv.v_z += n0 * (NODESPACE + NODEDEPTH) +
						    s * NODESHIFT;
						draw_node(node, 0);
					}
					if (st.st_opts & OP_SHOWMODS)
						draw_mod(&v, &mdim, &mf);
				}
				v.v_x -= (MODWIDTH + MODSPACE) * NMODS;
			}
			v.v_y -= (CAGEHEIGHT + CAGESPACE) * NCAGES;
		}
		v.v_x -= (CABWIDTH + CABSPACE) * NCABS;
	}
}

__inline void
draw_cluster_pipes(struct vec *v)
{
	float x, y, z, spx, spy, spz;
	float sx, sy, sz;
	struct vec *dimp;

	dimp = &vmodes[st.st_vmode].vm_ndim;
	spx = st.st_winspx;
	spy = st.st_winspy;
	spz = st.st_winspz;
	sx = dimp->v_w / 2;
	sy = dimp->v_h / 2;
	sz = dimp->v_d / 2;

	glPushMatrix();
	glTranslatef(v->v_x, v->v_y, v->v_z);

	/* Antialiasing */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(8.0);

	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 1.0f);
	for (x = sx; x < WI_WIDTH; x += spx)			/* z */
		for (y = sy; y < WI_HEIGHT; y += spy) {
			glVertex3f(x, y, -dimp->v_z / 2.0f);
			glVertex3f(x, y, WI_DEPTH);
		}

	glColor3f(0.0f, 1.0f, 0.0f);
	for (z = sz; z < WI_DEPTH; z += spz)			/* y */
		for (x = sx; x < WI_WIDTH; x += spx) {
			glVertex3f(x, -dimp->v_y / 2.0f, z);
			glVertex3f(x, WI_HEIGHT, z);
		}

	glColor3f(1.0f, 0.0f, 0.0f);
	for (y = sy; y < WI_HEIGHT; y += spy)
		for (z = sz; z < WI_DEPTH; z += spz) {		/* x */
			glVertex3f(-dimp->v_x / 2.0f, y, z);
			glVertex3f(WI_WIDTH, y, z);
		}
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
	glEnd();
	glPopMatrix();
}

__inline void
draw_cluster_wired(struct vec *v)
{
	int r, cb, cg, m, n;
	struct node *node;
	struct vec *nv;

	if (st.st_opts & OP_PIPES)
		draw_cluster_pipes(v);
	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						nv = &node->n_swiv;
						nv->v_x = node->n_wiv.v_x * st.st_winspx + v->v_x;
						nv->v_y = node->n_wiv.v_y * st.st_winspy + v->v_y;
						nv->v_z = node->n_wiv.v_z * st.st_winspz + v->v_z;
						node->n_v = nv;
						draw_node(node, 0);
					}
}

__inline void
draw_wired_frame(struct vec *vp, struct vec *dimp, struct fill *fillp)
{
	struct fill fill = *fillp;
	struct vec v;

	v = *vp;
	v.v_x -= st.st_winspx / 2.0f;
	v.v_y -= st.st_winspy / 2.0f;
	v.v_z -= st.st_winspz / 2.0f;

	glPushMatrix();
	glTranslatef(v.v_x, v.v_y, v.v_z);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
	fill.f_a = 0.2;
	draw_box_filled(dimp, &fill);
	glDisable(GL_BLEND);
	glPopMatrix();
}

/*
 * Draw the cluster repeatedly till we reach the clipping plane.
 * Since we can see MIN(WI_CLIP*) away, we must construct a 3D space
 * MIN(WI_CLIP*)^3 large and draw the cluster multiple times inside.
 */
__inline void
draw_clusters_wired(void)
{
	int xnum, znum, col;
	float x, y, z, adj;
	struct vec v, dim;

	clip = MIN3(WI_CLIPX, WI_CLIPY, WI_CLIPZ);

	x = st.st_x - clip;
	y = st.st_y - clip;
	z = st.st_z - clip;

	/* Snap to grid */
	adj = fmod(x, WI_WIDTH);
	if (adj < 0)
		adj += WI_WIDTH;
	x -= adj;

	adj = fmod(y, WI_HEIGHT);
	if (adj < 0)
		adj += WI_HEIGHT;
	y -= adj;

	adj = fmod(z, WI_DEPTH);
	if (adj < 0)
		adj += WI_DEPTH;
	z -= adj;

	wivstart.v_x = x;
	wivstart.v_y = y;
	wivstart.v_z = z;

	dim.v_w = WI_WIDTH;
	dim.v_h = WI_HEIGHT;
	dim.v_d = WI_DEPTH;

	xnum = (st.st_x + clip - x + WI_WIDTH - 1) / WI_WIDTH;
	znum = (st.st_z + clip - z + WI_DEPTH - 1) / WI_DEPTH;

	col = 0;
	for (v.v_y = y; v.v_y < st.st_y + clip; v.v_y += WI_HEIGHT) {
		for (v.v_z = z; v.v_z < st.st_z + clip; v.v_z += WI_DEPTH) {
			for (v.v_x = x; v.v_x < st.st_x + clip; v.v_x += WI_WIDTH) {
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

	wivdim.v_w = v.v_x - wivstart.v_x;
	wivdim.v_h = v.v_y - wivstart.v_y;
	wivdim.v_d = v.v_z - wivstart.v_z;
}

void
make_cluster(void)
{
	if (cluster_dl)
		glDeleteLists(cluster_dl, 1);
	cluster_dl = glGenLists(1);
	glNewList(cluster_dl, GL_COMPILE);

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		draw_cluster_physical();
		break;
	case VM_WIRED:
		draw_clusters_wired();
		break;
	case VM_WIREDONE: {
		struct vec v = { 0.0f, 0.0f, 0.0f };

		draw_cluster_wired(&v);
		break;
	    }
	}

	glEndList();
}
