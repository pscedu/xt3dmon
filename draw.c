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

extern double fmax(double, double);
extern double fmin(double, double);

#define TWEEN_MAX_POS	(2.0f)
#define TWEEN_MAX_LOOK	(0.04f)
#define TWEEN_THRES	(0.001f)
#define TWEEN_AMT	(0.05f)

struct vec wivstart, wivdim;

struct fill fill_black = { 0.0f, 0.0f, 0.0f, 1.0f, 0, 0 };
struct fill fill_grey  = { 0.2f, 0.2f, 0.2f, 1.0f, 0, 0 };
struct fill fill_light_blue = { 0.2f, 0.4f, 0.6f, 1.0f, 0, 0 };

__inline int
tween(__unused float start, float *cur, float stop, float max)
{
	float amt;

	if (stop != *cur) {
		amt = (stop - *cur) * TWEEN_AMT;
		amt = MIN(amt, max);
		*cur += amt;
		if (stop - *cur < TWEEN_THRES &&
		    stop - *cur > -TWEEN_THRES)
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

	if (st.st_rf) {
		rebuild(st.st_rf);
		st.st_rf = 0;
	}

	if (render_mode == RM_SELECT)
		sel_record_begin();

	if (st.st_vmode == VM_WIRED) {
#if 0
		struct vec lov, hiv;
		int clip;

		lov.v_x = st.st_x - WI_CLIPX;
		lov.v_y = st.st_y - WI_CLIPY;
		lov.v_z = st.st_z - WI_CLIPZ;
		hiv.v_x = st.st_x + WI_CLIPX; /* XXX:  + 1 (truncation) */
		hiv.v_y = st.st_y + WI_CLIPY;
		hiv.v_z = st.st_z + WI_CLIPZ;
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
	if (!TAILQ_EMPTY(&panels))
		draw_panels();

	glClearColor(0.0, 0.0, 0.2, 1.0);
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
draw_box_tex(struct vec *dim, struct fill *fillp)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float w = dim->v_w;
	float h = dim->v_h;
	float d = dim->v_d;
	float uw, uh, ud;
	float color[4];
	GLenum param;

	/* Number of times to tile image */
	uw = 0.5 * TILE_TEXTURE;
	ud = 1.0 * TILE_TEXTURE;
	uh = 1.0 * TILE_TEXTURE;

	glEnable(GL_TEXTURE_2D);

	if (st.st_opts & (OP_BLEND | OP_DIMNONSEL)) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		param = GL_BLEND;
	} else
		param = GL_REPLACE;

	glBindTexture(GL_TEXTURE_2D, fillp->f_texid);

	color[0] = fillp->f_r;
	color[1] = fillp->f_g;
	color[2] = fillp->f_b;
	color[3] = fillp->f_a;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, param);

	if (st.st_opts & (OP_BLEND | OP_DIMNONSEL))
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

	/* Polygon Offset */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 3.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

	if (st.st_opts & (OP_BLEND | OP_DIMNONSEL))
		glDisable(GL_BLEND);

	/* Disable Polygon Offset */
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_TEXTURE_2D);
}

/* Render a char from the font texture */
void draw_char(int ch, float x, float y, float z)
{
	if(ch < 0)
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
		if(j == 4)
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
	struct fill fill, *fp;

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

	if (n != selnode && selnode != NULL &&
	    st.st_opts & OP_DIMNONSEL) {
		/* Modify alpha for other nodes. */
		fill = *fp;
		fill.f_a = DIMMED_ALPHA;
		fp = &fill;
	}
	if (flags & NDF_NOOPTS)
		draw_box_filled(dimp, fp);
	else if (st.st_opts & OP_TEX)
		draw_box_tex(dimp, fp);
	else {
		if (st.st_opts & (OP_BLEND| OP_DIMNONSEL)) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
		}
		draw_box_filled(dimp, fp);
		if (st.st_opts & (OP_BLEND | OP_DIMNONSEL))
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
	for (x = sx; x <= WI_WIDTH + spx; x += spx)			/* z */
		for (y = sy; y <= WI_HEIGHT + spy; y += spy) {
			glVertex3f(x, y, 0.0f);
			glVertex3f(x, y, WI_DEPTH);
		}

	glColor3f(0.0f, 1.0f, 0.0f);
	for (z = sz; z <= WI_DEPTH + spz; z += spz)			/* y */
		for (x = sx; x <= WI_WIDTH + spx; x += spx) {
			glVertex3f(x, 0.0f, z);
			glVertex3f(x, WI_HEIGHT, z);
		}

	glColor3f(1.0f, 0.0f, 0.0f);
	for (y = sy; y <= WI_HEIGHT + spy; y += spy)
		for (z = sz; z <= WI_DEPTH + spz; z += spz) {		/* x */
			glVertex3f(0.0f, y, z);
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
	struct vec v, dim;

	v = *vp;
	v.v_x -= st.st_winspx / 2;
	v.v_y -= st.st_winspy / 2;
	v.v_z -= st.st_winspz / 2;

	dim = *dimp;
	dim.v_w += st.st_winspx;
	dim.v_h += st.st_winspy;
	dim.v_d += st.st_winspz;

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
 * Since we can see MIN(WI_CLIP*) away, we must construct a 3D space
 * MIN(WI_CLIP*)^3 large and draw the cluster multiple times inside.
 */
__inline void
draw_clusters_wired(void)
{
	int xnum, znum, col, adj, x, y, z;
	struct vec v, dim;

	x = st.st_x - WI_CLIPX;
	y = st.st_y - WI_CLIPY;
	z = st.st_z - WI_CLIPZ;

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

	xnum = (st.st_x + WI_CLIPX - x + WI_WIDTH - 1) / WI_WIDTH;
	znum = (st.st_z + WI_CLIPZ - z + WI_DEPTH - 1) / WI_DEPTH;

	col = 0;
	for (v.v_y = y; v.v_y < st.st_y + WI_CLIPY; v.v_y += WI_HEIGHT) {
		for (v.v_z = z; v.v_z < st.st_z + WI_CLIPZ; v.v_z += WI_DEPTH) {
			for (v.v_x = x; v.v_x < st.st_x + WI_CLIPX; v.v_x += WI_WIDTH) {
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

/*
 * The following two mathematical algorithms were created from
 * pseudocode found in "Fundamentals of Interactive Computer Graphics".
 */
void
RGB2HSV(struct fill *c)
{
	float r = c->f_r;
	float g = c->f_g;
	float b = c->f_b;
	float max, min, ran;
	float rc, gc, bc;

	max = fmax(fmax(r,g), b);
	min = fmin(fmin(r,g), b);
	ran = max - min;

	c->f_h = 0;
	c->f_s = 0;

	/* Value */
	c->f_v = max;

	/* Saturation */
	if (max != 0)
		c->f_s = ran / max;

	/* Hue */
	if (c->f_s != 0) {

		/* Measure color distances */
		rc = (max - r) / ran;
		gc = (max - g) / ran;
		bc = (max - b) / ran;

		/* Between yellow and magenta */
		if (r == max)
			c->f_h = bc - gc;
		/* Between cyan and yellow */
		else if (g == max)
			c->f_h = 2 + rc - bc;
		/* Between magenta and cyan */
		else if (b == max)
			c->f_h = 4 + gc - rc;

		/* Convert to degrees */
		c->f_h *= 60;

		if (c->f_h < 0)
			c->f_h += 360;
	}
}

void
HSV2RGB(struct fill *c)
{
	float s = c->f_s;
	float h = c->f_h;
	float v = c->f_v;
	float f, p, q, t;
	int i;

	if (s == 0)
		h = v;
	else {
		if (h == 360)
			h = 0;
		h /= 60;

		i = floorf(h);
		f = h - i;
		p = v * (1 - s);
		q = v * (1 - (s * f));
		t = v * (1 - (s * (1 - f)));

		switch (i) {
		case 0:	c->f_r = v; c->f_g = t; c->f_b = p; break;
		case 1: c->f_r = q; c->f_g = v; c->f_b = p; break;
		case 2: c->f_r = p; c->f_g = v; c->f_b = t; break;
		case 3: c->f_r = p; c->f_g = q; c->f_b = v; break;
		case 4:	c->f_r = t; c->f_g = p; c->f_b = v; break;
		case 5: c->f_r = v; c->f_g = p; c->f_b = q; break;
		}
	}
}

/*
 * Create a contrasting color by panning
 * 180 degrees around the color circle.
 */
void
rgb_contrast(struct fill *c)
{
	RGB2HSV(c);

	c->f_h -= 180;

	if (c->f_h < 0)
		c->f_h += 360;

	HSV2RGB(c);
}
