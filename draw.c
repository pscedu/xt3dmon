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

#define SELNODE_GAP	(0.1f)

struct fvec wivstart, wivdim;
float clip;

struct fill fill_black = { 0.0f, 0.0f, 0.0f, 1.0f, 0, 0 };
struct fill fill_grey  = { 0.2f, 0.2f, 0.2f, 1.0f, 0, 0 };
struct fill fill_light_blue = { 0.2f, 0.4f, 0.6f, 1.0f, 0, 0 };
struct fill selnodefill = { 0.20f, 0.40f, 0.60f, 1.00f, 0, 0 };		/* Dark blue */

void
draw(void)
{
	if (flyby_mode)
		flyby_update();

	if (render_mode == RM_SELECT)
		sel_record_begin();

	if (st.st_opts & OP_TWEEN) {
		/* XXX: benchmark to test static. */
		static struct fvec want, want_l;
		static struct fvec sc, sc_l;
		static float scale, scale_l;

		want.fv_w = want.fv_h = want.fv_d = 0.0f;
		want_l.fv_w = want_l.fv_h = want_l.fv_d = 0.0f;
		sc.fv_x = sc.fv_y = sc.fv_z = 1.0;
		sc_l.fv_x = sc_l.fv_y = sc_l.fv_z = 1.0;

		tween_probe(&st.st_x, tx, TWEEN_MAX_POS, &sc.fv_x, &want.fv_w);
		tween_probe(&st.st_y, ty, TWEEN_MAX_POS, &sc.fv_y, &want.fv_h);
		tween_probe(&st.st_z, tz, TWEEN_MAX_POS, &sc.fv_z, &want.fv_d);
		tween_probe(&st.st_lx, tlx, TWEEN_MAX_LOOK, &sc_l.fv_x, &want_l.fv_w);
		tween_probe(&st.st_ly, tly, TWEEN_MAX_LOOK, &sc_l.fv_y, &want_l.fv_h);
		tween_probe(&st.st_lz, tlz, TWEEN_MAX_LOOK, &sc_l.fv_z, &want_l.fv_d);

		scale = MIN3(sc.fv_x, sc.fv_y, sc.fv_z);
		scale_l = MIN3(sc_l.fv_x, sc_l.fv_y, sc_l.fv_z);

		tween_recalc(&st.st_x, tx, scale, want.fv_w);
		tween_recalc(&st.st_y, ty, scale, want.fv_h);
		tween_recalc(&st.st_z, tz, scale, want.fv_d);
		tween_recalc(&st.st_lx, tlx, scale_l, want_l.fv_w);
		tween_recalc(&st.st_ly, tly, scale_l, want_l.fv_h);
		tween_recalc(&st.st_lz, tlz, scale_l, want_l.fv_d);

		if (want.fv_w || want.fv_h || want.fv_d ||
		    want_l.fv_w || want_l.fv_h || want_l.fv_d)
			cam_update();
	}

	if (st.st_vmode == VM_WIRED)
		if (st.st_x + clip > wivstart.fv_x + wivdim.fv_w ||
		    st.st_x - clip < wivstart.fv_x ||
		    st.st_y + clip > wivstart.fv_y + wivdim.fv_h ||
		    st.st_y - clip < wivstart.fv_y ||
		    st.st_z + clip > wivstart.fv_z + wivdim.fv_d ||
		    st.st_z - clip < wivstart.fv_z) {
			panel_status_addinfo("Rebuild triggered\n");
			st.st_rf |= RF_CLUSTER;
		}

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
	int i = 0;

	/*
	** Parse the node id for use with
	** NODE0123456789 (so 4 letter gap before 0)
	*/
	nid = n->n_nid;
	while(nid >= 0 && i < MAX_CHARS) {
		list[MAX_CHARS-i-1] = 4 + nid % 10;
		nid /= 10;
		i++;
	}

	while(i < MAX_CHARS)
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

	for(i = 0; i < MAX_CHARS; i++) {
		/* -0.001 to place slightly in front */
		draw_char(list[i], -0.001, NODEDEPTH/2.0,
		    FONT_Z_OFFSET+FONT_DISPLACE_W*(float)(i));
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
	glColor3f(1.0f, 0.0f, 0.0f); 			/* x */
	glVertex3f(w - st.st_winsp.iv_x, h/2, d/2);
	glVertex3f(st.st_winsp.iv_x, h/2, d/2);

	glColor3f(0.0f, 1.0f, 0.0f);			/* y */
	glVertex3f(w/2, h - st.st_winsp.iv_y, d/2);
	glVertex3f(w/2, st.st_winsp.iv_y, d/2);

	glColor3f(0.0f, 0.0f, 1.0f);			/* z */
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
	struct fvec *vp, *dimp;
	struct fill *fp;
	GLenum param = GL_REPLACE;

	if (n->n_flags & NF_HIDE)
		return;

	fp = n->n_fillp;
	vp = n->n_v;
	dimp = &vmodes[st.st_vmode].vm_ndim;

	if ((flags & NDF_DONTPUSH) == 0) {
		glPushMatrix();
		glPushName(n->n_nid);
		glTranslatef(vp->fv_x, vp->fv_y, vp->fv_z);
	}

	if (st.st_opts & OP_BLEND) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		param = GL_BLEND;
	}

	if (st.st_opts & OP_TEX)
		draw_box_tex(dimp, fp, param);
	else
		draw_box_filled(dimp, fp);

	if (st.st_opts & OP_BLEND)
		glDisable(GL_BLEND);

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
	if (ground_dl)
		glDeleteLists(ground_dl, 1);
	ground_dl = glGenLists(1);
	glNewList(ground_dl, GL_COMPILE);

	glBegin(GL_QUADS);

	/* Ground */
	glColor3f(0.4f, 0.4f, 0.4f);
	switch (st.st_vmode) {
	case VM_WIREDONE:
		glVertex3f( -st.st_winsp.iv_x / 2.0f, -0.1f, -st.st_winsp.iv_z / 2.0f);
		glVertex3f( -st.st_winsp.iv_x / 2.0f, -0.1f, WI_DEPTH);
		glVertex3f(WI_WIDTH, -0.1f, WI_DEPTH);
		glVertex3f(WI_WIDTH, -0.1f, -st.st_winsp.iv_z / 2.0f);
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
	struct fvec mdim;
	struct fill mf;
	struct fvec v;

	mdim.fv_w = MODWIDTH + 0.02;
	mdim.fv_h = MODHEIGHT + 0.02;
	mdim.fv_d = MODDEPTH + 0.02;

	mf.f_r = 1.00;
	mf.f_g = 1.00;
	mf.f_b = 1.00;
	mf.f_a = 0.30;

	v.fv_x = v.fv_y = v.fv_z = NODESPACE;
	for (r = 0; r < NROWS; r++, v.fv_z += ROWDEPTH + ROWSPACE) {
		for (cb = 0; cb < NCABS; cb++, v.fv_x += CABWIDTH + CABSPACE) {
			for (cg = 0; cg < NCAGES; cg++, v.fv_y += CAGEHEIGHT + CAGESPACE) {
				for (m = 0; m < NMODS; m++, v.fv_x += MODWIDTH + MODSPACE) {
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						node->n_v = &node->n_physv;
						node->n_physv = v;

						s = n / (NNODES / 2);
						n0 = (n & 1) ^ ((n & 2) >> 1);

						node->n_physv.fv_y += s * (NODESPACE + NODEHEIGHT);
						node->n_physv.fv_z += n0 * (NODESPACE + NODEDEPTH) +
						    s * NODESHIFT;
						draw_node(node, 0);
					}
					if (st.st_opts & OP_SHOWMODS)
						draw_mod(&v, &mdim, &mf);
				}
				v.fv_x -= (MODWIDTH + MODSPACE) * NMODS;
			}
			v.fv_y -= (CAGEHEIGHT + CAGESPACE) * NCAGES;
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
	glColor3f(0.0f, 0.0f, 1.0f);
	for (x = sx; x < WI_WIDTH; x += spx)			/* z */
		for (y = sy; y < WI_HEIGHT; y += spy) {
			glVertex3f(x, y, -dimp->fv_z / 2.0f);
			glVertex3f(x, y, WI_DEPTH);
		}

	glColor3f(0.0f, 1.0f, 0.0f);
	for (z = sz; z < WI_DEPTH; z += spz)			/* y */
		for (x = sx; x < WI_WIDTH; x += spx) {
			glVertex3f(x, -dimp->fv_y / 2.0f, z);
			glVertex3f(x, WI_HEIGHT, z);
		}

	glColor3f(1.0f, 0.0f, 0.0f);
	for (y = sy; y < WI_HEIGHT; y += spy)
		for (z = sz; z < WI_DEPTH; z += spz) {		/* x */
			glVertex3f(-dimp->fv_x / 2.0f, y, z);
			glVertex3f(WI_WIDTH, y, z);
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
						nv->fv_x = node->n_wiv.fv_x * st.st_winsp.iv_x + v->fv_x;
						nv->fv_y = node->n_wiv.fv_y * st.st_winsp.iv_y + v->fv_y;
						nv->fv_z = node->n_wiv.fv_z * st.st_winsp.iv_z + v->fv_z;
						node->n_v = nv;
						draw_node(node, 0);
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
 * Since we can see MIN(WI_CLIP*) away, we must construct a 3D space
 * MIN(WI_CLIP*)^3 large and draw the cluster multiple times inside.
 */
__inline void
draw_clusters_wired(void)
{
	int xnum, znum, col;
	float x, y, z;
	struct fvec v, dim;

	clip = MIN3(WI_CLIPX, WI_CLIPY, WI_CLIPZ);

	x = st.st_x - clip;
	y = st.st_y - clip;
	z = st.st_z - clip;

	x -= snap_to_grid(x, WI_WIDTH, 0.0);
	y -= snap_to_grid(y, WI_HEIGHT, 0.0);
	z -= snap_to_grid(z, WI_DEPTH, 0.0);

	wivstart.fv_x = x;
	wivstart.fv_y = y;
	wivstart.fv_z = z;

	dim.fv_w = WI_WIDTH;
	dim.fv_h = WI_HEIGHT;
	dim.fv_d = WI_DEPTH;

	xnum = (st.st_x + clip - x + WI_WIDTH - 1) / WI_WIDTH;
	znum = (st.st_z + clip - z + WI_DEPTH - 1) / WI_DEPTH;

	col = 0;
	for (v.fv_y = y; v.fv_y < st.st_y + clip; v.fv_y += WI_HEIGHT) {
		for (v.fv_z = z; v.fv_z < st.st_z + clip; v.fv_z += WI_DEPTH) {
			for (v.fv_x = x; v.fv_x < st.st_x + clip; v.fv_x += WI_WIDTH) {
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
	if (select_dl)
		glDeleteLists(select_dl, 1);
	if (SLIST_EMPTY(&selnodes)) {
		select_dl = 0;
		return;
	}

	select_dl = glGenLists(1);
	glNewList(select_dl, GL_COMPILE);

	vmodes[st.st_vmode].vm_ndim.fv_w += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_h += SELNODE_GAP * 2;
	vmodes[st.st_vmode].vm_ndim.fv_d += SELNODE_GAP * 2;

	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;
		ofp = n->n_fillp;
		n->n_fillp = &selnodefill;

		switch (st.st_vmode) {
		case VM_WIRED:
			pos.fv_x = n->n_wiv.fv_x * st.st_winsp.iv_x + wivstart.fv_x - SELNODE_GAP;
			pos.fv_y = n->n_wiv.fv_y * st.st_winsp.iv_y + wivstart.fv_y - SELNODE_GAP;
			pos.fv_z = n->n_wiv.fv_z * st.st_winsp.iv_z + wivstart.fv_z - SELNODE_GAP;
			for (v.fv_x = 0.0f; v.fv_x < wivdim.fv_w; v.fv_x += WI_WIDTH)
				for (v.fv_y = 0.0f; v.fv_y < wivdim.fv_h; v.fv_y += WI_HEIGHT)
					for (v.fv_z = 0.0f; v.fv_z < wivdim.fv_d; v.fv_z += WI_DEPTH) {
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
