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

#include "cdefs.h"
#include "mon.h"

#define TWEEN_MAX_POS	(2.0f)
#define TWEEN_MAX_LOOK	(0.01f)
#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.05f)

struct vec wivstart, wivdim;

struct fill fill_black = { 0.0f, 0.0f, 0.0f, 1.0f, 0, 0 };
struct fill fill_grey  = { 0.2f, 0.2f, 0.2f, 1.0f, 0, 0 };
struct fill fill_light_blue  = { 0.2f, 0.4f, 0.6f, 1.0f, 0, 0 };

__inline int
tween(__unused float start, float *cur, float stop, float max)
{
	float amt;

	if (stop != *cur) {
		amt = (stop - *cur) * TWEEN_AMT;
		amt = MIN(amt, max);
		*cur += amt;
		if (fabs(stop - *cur) < TWEEN_THRES)
			*cur = stop;
		return (1);
	}
	return (0);
}

void
draw(void)
{
	if (active_flyby || build_flyby)
		update_flyby();

	if (render_mode == RM_SELECT)
		sel_record_begin();

	if (st.st_vmode == VM_WIRED) {
#if 0
		struct vec lov, hiv;
		int clip;

		lov.v_x = st.st_x - WI_CLIP;
		lov.v_y = st.st_y - WI_CLIP;
		lov.v_z = st.st_z - WI_CLIP;
		hiv.v_x = st.st_x + WI_CLIP; /* XXX:  + 1 (truncation) */
		hiv.v_y = st.st_y + WI_CLIP;
		hiv.v_z = st.st_z + WI_CLIP;
		if (lov.v_x < wivstart.v_x ||
		    lov.v_y < wivstart.v_y ||
		    lov.v_z < vstart.v_z ||
		    hiv.v_x > wivstart.v_x + wivdim.v_x ||
		    hiv.v_y > wivstart.v_y + wivdim.v_y ||
		    hiv.v_z > wivstart.v_z + wivdim.v_z)
			rebuild(RF_COMPILE);
#endif
	}

	if (st.st_opts & OP_TWEEN)
		if (tween(ox, &st.st_x, tx, TWEEN_MAX_POS) |
		    tween(oy, &st.st_y, ty, TWEEN_MAX_POS) |
		    tween(oz, &st.st_z, tz, TWEEN_MAX_POS) |
		    tween(olx, &st.st_lx, tlx, TWEEN_MAX_LOOK) |
		    tween(oly, &st.st_ly, tly, TWEEN_MAX_LOOK) |
		    tween(olz, &st.st_lz, tlz, TWEEN_MAX_LOOK))
			cam_update();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (st.st_opts & OP_GROUND)
		glCallList(ground_dl);
	glCallList(cluster_dl);
	if (select_dl)
		glCallList(select_dl);
	draw_panels();

#if 0
	// DEBUG LINES
	if(st.st_opts & OP_DEBUG)
	{
		glColor3f(0.5, 0.5, 0.5);
		glLineWidth(2.0);
		glBegin(GL_LINES);
		glVertex3f(gLines[0].x, gLines[0].y, gLines[0].z);
		glVertex3f(gLines[1].x, gLines[1].y, gLines[1].z);
		glVertex3f(gLines[2].x, gLines[2].y, gLines[2].z);
		glVertex3f(gLines[3].x, gLines[3].y, gLines[3].z);
		glEnd();

		glPointSize(5.0);
		glBegin(GL_POINTS);
		glVertex3f(gPoints[0].x, gPoints[0].y, gPoints[0].z);
		glVertex3f(gPoints[1].x, gPoints[1].y, gPoints[1].z);
		glEnd();
	}
#endif

	glClearColor(0.0, 0.0, 0.0, 1.0);
	if (st.st_opts & OP_CAPTURE || gDebugCapture)
		capture_fb(capture_mode);
	if (render_mode == RM_SELECT)
		sel_record_end();
	else if (st.st_opts & OP_DISPLAY)
		glutSwapBuffers();
}

/*
 *	y			12
 *     / \	    +-----------------------+ (x+w,y+h,z+d)
 *	|	   /			   /|
 *	|      10 / |		       11 / |
 *	|	 /  	    9		 /  |
 *	|	+-----------------------+   |
 *	|	|   			|   | 7
 *	|	|   | 8			|   |
 *	|	|   			|   |
 *	|	| 5 |			| 6 |
 *	|	|   		4	|   |
 *	|	|   + - - - - - - - - - | - + (x+w,y,z+d)
 *	|	|  /			|  /
 *	|	| / 2			| / 3
 *	|	|/			|/
 *	|	+-----------------------+
 *	|    (x,y,z)	   1	     (x,y,z+d)
 *	|
 *	+----------------------------------------------->z
 *     /
 *    /
 *  |_
 *  x
 */
void
draw_box_outline(const struct vec *dim, const struct fill *fillp)
{
	float w = dim->v_w, h = dim->v_h, d = dim->v_d;
	float x, y, z;

	x = y = z = 0.0f;

	/* Wireframe outline */
	x -= WFRAMEWIDTH;
	y -= WFRAMEWIDTH;
	z -= WFRAMEWIDTH;
	w += 2.0f * WFRAMEWIDTH;
	h += 2.0f * WFRAMEWIDTH;
	d += 2.0f * WFRAMEWIDTH;

	/* Antialiasing */
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(1.0);
	glColor4f(fillp->f_r, fillp->f_g, fillp->f_b, fillp->f_a);
	glBegin(GL_LINES);

	/* Back */
	glVertex3f(x, y, z);			/* 1 */
	glVertex3f(x, y, z+d);

	glVertex3f(x, y, z);			/* 2 */
	glVertex3f(x+w, y, z);

	glVertex3f(x, y, z+d);			/* 3 */
	glVertex3f(x+w, y, z+d);

	glVertex3f(x+w, y, z);			/* 4 */
	glVertex3f(x+w, y, z+d);

	glVertex3f(x, y, z);			/* 5 */
	glVertex3f(x, y+h, z);

	glVertex3f(x, y, z+d);			/* 6 */
	glVertex3f(x, y+h, z+d);

	glVertex3f(x+w, y, z+d);		/* 7 */
	glVertex3f(x+w, y+h, z+d);

	glVertex3f(x+w, y, z);			/* 8 */
	glVertex3f(x+w, y+h, z);

	glVertex3f(x, y+h, z);			/* 9 */
	glVertex3f(x, y+h, z+d);

	glVertex3f(x, y+h, z);			/* 10 */
	glVertex3f(x+w, y+h, z);

	glVertex3f(x, y+h, z+d);		/* 11 */
	glVertex3f(x+w, y+h, z+d);

	glVertex3f(x+w, y+h, z);		/* 12 */
	glVertex3f(x+w, y+h, z+d);

	glEnd();
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

void
draw_box_filled(const struct vec *dim, const struct fill *fillp)
{
	float w = dim->v_w, h = dim->v_h, d = dim->v_d;
	float x, y, z;

	x = y = z = 0.0f;
	glColor4f(fillp->f_r, fillp->f_g, fillp->f_b, fillp->f_a);
	glBegin(GL_QUADS);

	/* Back */
	glVertex3f(x, y, z);
	glVertex3f(x, y+h, z);
	glVertex3f(x+w, y+h, z);
	glVertex3f(x+w, y, z);

	/* Front */
	glVertex3f(x, y, z+d);
	glVertex3f(x, y+h, z+d);
	glVertex3f(x+w, y+h, z+d);
	glVertex3f(x+w, y, z+d);

	/* Right */
	glVertex3f(x+w, y, z);
	glVertex3f(x+w, y, z+d);
	glVertex3f(x+w, y+h, z+d);
	glVertex3f(x+w, y+h, z);

	/* Left */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+d);
	glVertex3f(x, y+h, z+d);
	glVertex3f(x, y+h, z);

	/* Top */
	glVertex3f(x, y+h, z);
	glVertex3f(x, y+h, z+d);
	glVertex3f(x+w, y+h, z+d);
	glVertex3f(x+w, y+h, z);

	/* Bottom */
	glVertex3f(x, y, z);
	glVertex3f(x, y, z+d);
	glVertex3f(x+w, y, z+d);
	glVertex3f(x+w, y, z);

	glEnd();
}

__inline void
draw_node_tex(struct node *n)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float w = NODEWIDTH;
	float h = NODEHEIGHT;
	float d = NODEDEPTH;
	float uw, uh, ud;
	float color[4];
	GLenum param;

	/* Convert to texture units */
	uw = 1.0;
	ud = 2.0;
	uh = 2.0;

	glEnable(GL_TEXTURE_2D);

	if (st.st_opts & OP_BLEND){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		param = GL_BLEND;
	} else
		param = GL_REPLACE;

	glBindTexture(GL_TEXTURE_2D, jstates[n->n_state].js_fill.f_texid);

	color[0] = n->n_fillp->f_r;
	color[1] = n->n_fillp->f_g;
	color[2] = n->n_fillp->f_b;
	color[3] = n->n_fillp->f_a;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, param);

	if (st.st_opts & OP_BLEND)
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

	/* Polygon Offset */
	if(st.st_opts & OP_POLYOFFSET) {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 3.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glBegin(GL_QUADS);

	/* Back */
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(uh, uw);
	glVertex3f(x+w, y, z);
	glTexCoord2f(uh, 0.0);

	/* Front */
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(uh, uw);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(uh, 0.0);

	/* Right */
	glVertex3f(x+w, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(0.0, ud);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(uh, ud);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(uh, 0.0);

	/* Left */
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, ud);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(uh, ud);
	glVertex3f(x, y+h, z);
	glTexCoord2f(uh, 0.0);

	/* Top */
	glVertex3f(x, y+h, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y+h, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y+h, z+d);
	glTexCoord2f(ud, uw);
	glVertex3f(x+w, y+h, z);
	glTexCoord2f(ud, 0.0);

	/* Bottom */
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z+d);
	glTexCoord2f(0.0, uw);
	glVertex3f(x+w, y, z+d);
	glTexCoord2f(ud, uw);
	glVertex3f(x+w, y, z);
	glTexCoord2f(ud, 0.0);

	glEnd();

	if (st.st_opts & OP_BLEND)
		glDisable(GL_BLEND);

	/* Disable Polygon Offset */
	if(st.st_opts & OP_POLYOFFSET) {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_TEXTURE_2D);
}

__inline void
draw_node_label(struct node *n)
{
	char *s, buf[BUFSIZ];

	glColor3f(1.0f, 1.0f, 0.0f);
	glRasterPos3f(n->n_v->v_x, n->n_v->v_y, n->n_v->v_z);

	snprintf(buf, sizeof(buf), "NID: %d", n->n_nid);

	for (s = buf; *s != '\0'; s++)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
}

#define SELNODE_GAP (0.01f)

__inline void
draw_node(struct node *n)
{
	struct vec v, *vp, dim, *dimp;

	if (n->n_hide)
		return;

	if (selnode == n) {
		v = *n->n_v;
		v.v_x -= SELNODE_GAP;
		v.v_y -= SELNODE_GAP;
		v.v_z -= SELNODE_GAP;
		vp = &v;

		dim = vmodes[st.st_vmode].vm_ndim;
		vmodes[st.st_vmode].vm_ndim.v_w += SELNODE_GAP * 2;
		vmodes[st.st_vmode].vm_ndim.v_h += SELNODE_GAP * 2;
		vmodes[st.st_vmode].vm_ndim.v_d += SELNODE_GAP * 2;
		dimp = &dim;
	} else {
		vp = n->n_v;
		dimp = &vmodes[st.st_vmode].vm_ndim;
	}

	glPushMatrix();
	glPushName(n->n_nid);
	glTranslatef(vp->v_x, vp->v_y, vp->v_z);
	if (st.st_opts & OP_TEX)
		draw_node_tex(n);
	else {
		if (st.st_opts & OP_BLEND) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		}
		draw_box_filled(dimp, n->n_fillp);
		if (st.st_opts & OP_BLEND)
			glDisable(GL_BLEND);
	}
	if (st.st_opts & OP_WIREFRAME)
		draw_box_outline(dimp, &fill_black);
	if (st.st_opts & OP_NLABELS)
		draw_node_label(n);
	glPopName();
	glPopMatrix();
}

__inline void
draw_mod(struct vec *v, struct vec *dim, struct fill *fillp)
{
	glPushMatrix();
	glTranslatef(v->v_x, v->v_y, v->v_z);

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
		glVertex3f( -5.0f, -0.1f, -5.0f);
		glVertex3f( -5.0f, -0.1f, WI_DEPTH + 5.0f);
		glVertex3f(WI_WIDTH + 5.0f, -0.1f, WI_DEPTH + 5.0f);
		glVertex3f(WI_WIDTH + 5.0f, -0.1f, -5.0f);
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

	mdim.v_w = MODWIDTH;
	mdim.v_h = MODHEIGHT;
	mdim.v_d = MODDEPTH;

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
						draw_node(node);
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
draw_cluster_wired(struct vec *v)
{
	int r, cb, cg, m, n;
	struct node *node;
	struct vec *nv;

	for (r = 0; r < NROWS; r++)
		for (cb = 0; cb < NCABS; cb++)
			for (cg = 0; cg < NCAGES; cg++)
				for (m = 0; m < NMODS; m++)
					for (n = 0; n < NNODES; n++) {
						node = &nodes[r][cb][cg][m][n];
						nv = &node->n_swiv;
						nv->v_x = node->n_wiv.v_x * st.st_winsp + v->v_x;
						nv->v_y = node->n_wiv.v_y * st.st_winsp + v->v_y;
						nv->v_z = node->n_wiv.v_z * st.st_winsp + v->v_z;
						node->n_v = nv;
						draw_node(node);
					}
}

__inline void
draw_wired_frame(struct vec *vp, struct vec *dimp, struct fill *fillp)
{
	struct fill fill = *fillp;
	struct vec v, dim;

	v = *vp;
	v.v_x -= st.st_winsp / 2;
	v.v_y -= st.st_winsp / 2;
	v.v_z -= st.st_winsp / 2;

	dim = *dimp;
	dim.v_w += st.st_winsp;
	dim.v_h += st.st_winsp;
	dim.v_d += st.st_winsp;

	glPushMatrix();
	glTranslatef(v.v_x, v.v_y, v.v_z);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
	fill.f_a = 0.2;
	draw_box_filled(&dim, &fill);
	glDisable(GL_BLEND);
	glPopMatrix();
}

/*
 * Draw the cluster repeatedly till we reach the clipping plane.
 * Since we can see WI_CLIP away, we must construct a 3D space
 * WI_CLIP^3 large and draw the cluster multiple times inside.
 */
__inline void
draw_clusters_wired(void)
{
	int xnum, znum, col, adj, x, y, z;
	struct vec v, dim;

	x = st.st_x - WI_CLIP;
	y = st.st_y - WI_CLIP;
	z = st.st_z - WI_CLIP;

	/* Snap to grid */
	adj = x % WI_WIDTH;
	if (adj < 0)
		adj += WI_WIDTH;
	x -= adj;

	adj = y % WI_HEIGHT;
	if (adj < 0)
		adj += WI_HEIGHT;
	y -= adj;

	adj = z % WI_DEPTH;
	if (adj < 0)
		adj += WI_DEPTH;
	z -= adj;

	wivstart = v;

	dim.v_w = WI_WIDTH;
	dim.v_h = WI_HEIGHT;
	dim.v_d = WI_DEPTH;

	xnum = (st.st_x + WI_CLIP - x + WI_WIDTH - 1) / WI_WIDTH;
	znum = (st.st_z + WI_CLIP - z + WI_DEPTH - 1) / WI_DEPTH;

printf("x: %d, z: %d\n", xnum, znum);

	col = 0;
	for (v.v_y = y; v.v_y < st.st_y + WI_CLIP; v.v_y += WI_HEIGHT) {
		for (v.v_z = z; v.v_z < st.st_z + WI_CLIP; v.v_z += WI_DEPTH) {
			for (v.v_x = x; v.v_x < st.st_x + WI_CLIP; v.v_x += WI_WIDTH) {
		//		draw_cluster_wired(&v);
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
