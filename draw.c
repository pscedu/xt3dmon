/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cdefs.h"
#include "cam.h"
#include "draw.h"
#include "env.h"
#include "fill.h"
#include "lnseg.h"
#include "mark.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "phys.h"
#include "queue.h"
#include "route.h"
#include "selnode.h"
#include "state.h"
#include "util.h"
#include "xmath.h"

#define SELNODE_IF_NEEDED(n, selpipes) \
	((selpipes) ? (n)->n_flags & NF_SELNODE : 1)

#define SKEL_GAP	(0.1f)

float	 snap_to_grid(float, float, float);

struct fvec wi_repstart;
struct fvec wi_repdim;
float clip;

GLUquadric *quadric;

unsigned int dl_cluster[2];
unsigned int dl_ground[2];
unsigned int dl_selnodes[2];

__inline void
draw_compass(int u, __unused int w, int v, __unused int h)
{
	double cl, mvm[16], pvm[16], x, y, z;
	struct fill *fp;
	struct fvec fv;
	int vp[4];

	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
	glGetDoublev(GL_PROJECTION_MATRIX, pvm);

	if (gluUnProject((double)u, (double)v, 0.0,
	    mvm, pvm, vp, &x, &y, &z) == GL_FALSE)
		return;

	/*
	 * The unprojected point will be at depth z=0,
	 * and if we try to draw there, half of the line
	 * will not be visible, so scale it out.
	 */
	fv.fv_x = st.st_v.fv_x + 1.5 * (x - st.st_v.fv_x);
	fv.fv_y = st.st_v.fv_y + 1.5 * (y - st.st_v.fv_y);
	fv.fv_z = st.st_v.fv_z + 1.5 * (z - st.st_v.fv_z);

	glPushMatrix();

	gluQuadricDrawStyle(quadric, GLU_FILL);

	/* Anti-aliasing */
	glLineWidth(0.6f);
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	cl = 0.05;
#define CMP_ATHK (0.005)
#define CMP_ALEN (0.01)

	/* Draw z axis. */
	fp = &fill_dim[DIM_Z];
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z - cl / 2.0);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, 0.0, cl);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z + cl / 2.0 - CMP_ALEN);
	gluCylinder(quadric, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glRasterPos3d(fv.fv_x, fv.fv_y, fv.fv_z + cl / 2.0);
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '+');
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, 'z');

	/* Draw y axis. */
	fp = &fill_dim[DIM_Y];
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y - cl / 2.0, fv.fv_z);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(0.0, cl, 0.0);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(fv.fv_x, fv.fv_y + cl / 2.0 - CMP_ALEN, fv.fv_z);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(quadric, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glRasterPos3d(fv.fv_x, fv.fv_y + cl / 2.0, fv.fv_z);
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '+');
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, 'y');

	/* Draw x axis. */
	fp = &fill_dim[DIM_X];
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	glPushMatrix();
	glTranslatef(fv.fv_x - cl / 2.0, fv.fv_y, fv.fv_z);
	glBegin(GL_LINES);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(cl, 0.0, 0.0);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(fv.fv_x + cl / 2.0 - CMP_ALEN, fv.fv_y, fv.fv_z);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	gluCylinder(quadric, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glRasterPos3d(fv.fv_x + cl / 2.0, fv.fv_y, fv.fv_z);
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '+');
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, 'x');

	glPopMatrix();

	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

__inline void
draw_string(int u, int v, int h, void *font, const unsigned char *s)
{
	const unsigned char *t;
	int margin, w;

	/* Save state and set things up for 2D. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gluOrtho2D(0.0, winv.iv_w, 0.0, winv.iv_h);
	glColor4f(1.0f, 1.0f, 1.0f, 0.9f);

	w = glutBitmapLength(font, s);

	glRasterPos2f(u, v);
	for (t = s; *t != '\0'; t++)
		glutBitmapCharacter(font, *t);

	u++;
	v--;
	glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
	glRasterPos2f(u, v);
	for (t = s; *t != '\0'; t++)
		glutBitmapCharacter(font, *t);

	margin = 2;
	u -= 1 + margin;
	w += margin * 2;
	h += margin * 2;
	v += h - 4 - margin;

	glColor4f(0.2f, 0.2f, 0.2f, 0.6f);
	glBegin(GL_QUADS);
	glVertex2d(u,		v);
	glVertex2d(u + w,	v);
	glVertex2d(u + w,	v - h + 1);
	glVertex2d(u,		v - h + 1);
	glEnd();

	glDisable(GL_BLEND);

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

__inline void
draw_caption(void)
{
	const unsigned char *s;
	const char *cap;
	void *font;
	int u, w, h;

	font = GLUT_BITMAP_TIMES_ROMAN_24;
	h = 24;

	/* Draw the caption text, center on x */
	cap = caption_get();
	if (cap) {
		s = (const unsigned char *)cap;
		w = glutBitmapLength(font, s);
		u = (winv.iv_w / 2.0) - (w / 2.0);
		draw_string(u, 10, h, font, s);
	}
#if 0
	s = (const unsigned char *)"Live BigBen/XT3 Monitor";
	w = glutBitmapLength(font, s);
	u = (winv.iv_w / 2.0) - (w / 2.0);
	draw_string(u, winv.iv_h - h - 3, h, font, s);
#endif
}

#define FTX_TWIDTH	2048	/* Must be power of 2. */
#define FTX_THEIGHT	64	/* Must be power of 2. */
#define FTX_CWIDTH	17
#define FTX_CHEIGHT	34
#define FTX_CSTART	33
#define FTX_CEND	126

/*
 * Texture map from font texture of size (FTX_TWIDTH,FTX_THEIGHT)
 * with chars of size (FTX_CWIDTH,FTX_CHEIGHT) ranging between
 * ASCII chars FTX_CSTART and FTX_CEND.
 */
void
draw_text(const char *buf, struct fvec *dim, struct fill *fp)
{
	double uscale, vscale;
	double z, d, h, y;
	const char *s;
	char c;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, fill_font.f_texid_a[wid]);

	uscale = FTX_TWIDTH / (double)FTX_CWIDTH;
	vscale = FTX_THEIGHT / (double)FTX_CHEIGHT;

	glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
	glBegin(GL_QUADS);

	z = 0.0;
	d = dim->fv_w / strlen(buf);

	h = dim->fv_h / strlen(buf);//* FTX_CWIDTH * d / FTX_CHEIGHT;
	y = dim->fv_h / 2.0 - h / 2.0;

	for (s = buf; *s != '\0'; s++, z += d) {
		if (*s < FTX_CSTART || *s > FTX_CEND)
			/* XXX: draw '?'? */
			continue;
		c = *s - FTX_CSTART;

		glTexCoord2d(c / uscale, 1.0 / vscale);
		glVertex3d(0.0, y, z);
		glTexCoord2d(c / uscale, 1.0);
		glVertex3d(0.0, y + h, z);
		glTexCoord2d((c + 1) / uscale, 1.0);
		glVertex3d(0.0, y + h, z + d);
		glTexCoord2d((c + 1) / uscale, 1.0 / vscale);
		glVertex3d(0.0, y, z + d);
	}

	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

__inline void
draw_node_label(struct node *n, struct fill *fp)
{
	char buf[BUFSIZ];
	struct fvec dim;
	struct fill c;

	c = *fp;
	if ((c.f_flags & FF_SKEL) == 0)
		fill_contrast(&c);

	dim.fv_w = n->n_dimp->fv_d;
	dim.fv_h = n->n_dimp->fv_h;

	snprintf(buf, sizeof(buf), "%04d", n->n_nid);

	glPushMatrix();
	glTranslatef(-0.001, 0.0, 0.0);
	draw_text(buf, &dim, &c);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(n->n_dimp->fv_w + 0.001, 0.0, n->n_dimp->fv_d);
	dim.fv_w *= -1.0;
	draw_text(buf, &dim, &c);
	glPopMatrix();
}

/*
 * Move the node position (for animation).
 */
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
	struct fvec *fvp;
	struct fill *fp;

/*
	if (!node_show(n))
		return;

	if (st.st_opts & OP_SUBSET &&
	    (n->n_flags & NF_SHOW) == 0)
		return;
*/

	if (st.st_opts & OP_NODEANIM &&
	    st.st_vmode != VM_WIRED) {
		fvp = &n->n_vcur;
		/*
		 * node_tween_dir only updates one direction at a time,
		 * so we use a bitwise OR operator so they all run.
		 */
		if ((flags & NDF_NOTWEEN) == 0 &&
		    (node_tween_dir(&n->n_vcur.fv_x, &n->n_v->fv_x) |
		    node_tween_dir(&n->n_vcur.fv_y, &n->n_v->fv_y) |
		    node_tween_dir(&n->n_vcur.fv_z, &n->n_v->fv_z)))
			st.st_rf |= RF_CLUSTER | RF_SELNODE;
	} else
		fvp = n->n_v;
	glPushMatrix();
	glTranslatef(fvp->fv_x, fvp->fv_y, fvp->fv_z);

	if (n->n_flags & NF_SELNODE)
		fp = &fill_selnode;
	else
		fp = n->n_fillp;

	switch (n->n_geom) {
	case GEOM_CUBE:
		draw_cube(n->n_dimp, fp, DF_FRAME);
		break;
	case GEOM_SPHERE:
		draw_sphere(n->n_dimp, fp, DF_FRAME);
		break;
	}

	if (st.st_opts & OP_NLABELS ||
	    (n->n_flags & NF_SELNODE &&
	    st.st_opts & OP_SELNLABELS))
		draw_node_label(n, fp);

	glPopMatrix();
}

__inline void
draw_ground(void)
{
	struct fvec fv, fdim, *ndim;
	struct fill *fp;

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(1.0);
	glBegin(GL_LINES);

	fp = &fill_dim[DIM_X];
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(-500.0f, 0.0f, 0.0f);
	glVertex3f(500.0f, 0.0f, 0.0f);

	fp = &fill_dim[DIM_Y];
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	glVertex3f(0.0f, -500.0f, 0.0f);
	glVertex3f(0.0f, 500.0f, 0.0f);

	fp = &fill_dim[DIM_Z];
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	glVertex3f(0.0f, 0.0f, -500.0f);
	glVertex3f(0.0f, 0.0f, 500.0f);
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);

	/* Ground */
	switch (st.st_vmode) {
	case VM_WIONE:
		fv.fv_x = st.st_winsp.iv_x * (st.st_wioff.iv_x - 1);
		fv.fv_y = -0.2f + st.st_wioff.iv_y * st.st_winsp.iv_y;
		fv.fv_z = st.st_winsp.iv_z * (st.st_wioff.iv_z - 1);

		ndim = &vmodes[VM_WIONE].vm_ndim[GEOM_CUBE];
		fdim.fv_w = (widim.iv_w + 1) * st.st_winsp.iv_w + ndim->fv_w;
		fdim.fv_y = -0.2f / 2.0f;
		fdim.fv_d = (widim.iv_d + 1) * st.st_winsp.iv_d + ndim->fv_d;

		glPushMatrix();
		glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z);
		draw_cube(&fdim, &fill_ground, DF_FRAME);
		glPopMatrix();
		break;
	case VM_PHYS:
		fv.fv_x = -5.0f;
		fv.fv_y = -0.2f;
		fv.fv_z = -5.0f;

		fdim.fv_w = physdim_top->pd_size.fv_w - 2 * fv.fv_x;
		fdim.fv_h = -fv.fv_y / 2.0f;
		fdim.fv_d = 2 * physdim_top->pd_size.fv_d +
		    physdim_top->pd_space - 2 * fv.fv_z;

		glPushMatrix();
		glTranslatef(fv.fv_x, fv.fv_y, fv.fv_z);
		draw_cube(&fdim, &fill_ground, DF_FRAME);
		glPopMatrix();
		break;
	}
}

/*
 * Recursively draw all physical dimension
 * (row, cab, cage, etc.) skeletons.
 */
__inline void
draw_pd_skel(struct physdim *pdp, struct physdim *targetpd,
    struct fvec *fvp)
{
	struct fvec fv;
	size_t i;

	fv = *fvp;
	if (pdp == targetpd) {
		for (i = 0; i < targetpd->pd_mag; i++) {
			glPushMatrix();
			glTranslatef(fv.fv_x,
			    fv.fv_y, fv.fv_z);
			draw_cube(&targetpd->pd_size,
			    &fill_clskel, 0);
			glPopMatrix();

			fv.fv_val[pdp->pd_spans] +=
			    pdp->pd_size.fv_val[pdp->pd_spans] +
			    pdp->pd_space;
			fv.fv_x += pdp->pd_offset.fv_x;
			fv.fv_y += pdp->pd_offset.fv_y;
			fv.fv_z += pdp->pd_offset.fv_z;
		}
	} else {
		for (i = 0; i < pdp->pd_mag; i++) {
			draw_pd_skel(pdp->pd_contains, targetpd, &fv);
			fv.fv_val[pdp->pd_spans] +=
			    pdp->pd_size.fv_val[pdp->pd_spans] +
			    pdp->pd_space;
			fv.fv_x += pdp->pd_offset.fv_x;
			fv.fv_y += pdp->pd_offset.fv_y;
			fv.fv_z += pdp->pd_offset.fv_z;
		}
	}
}

void
draw_skel(void)
{
	struct fvec cldim, fv, dim, *ndim;
	struct physdim *pd;

	switch (st.st_vmode) {
	case VM_PHYS:
		for (pd = physdim_top; pd != NULL; pd = pd->pd_contains)
			if (pd->pd_flags & PDF_SKEL) {
				vec_set(&fv, NODESPACE, NODESPACE, NODESPACE);
				draw_pd_skel(physdim_top, pd, &fv);
			}
		break;
	case VM_WIRED:
	case VM_WIONE:
		cldim.fv_w = (widim.iv_w - 1) * st.st_winsp.iv_x;
		cldim.fv_h = (widim.iv_h - 1) * st.st_winsp.iv_y;
		cldim.fv_d = (widim.iv_d - 1) * st.st_winsp.iv_z;

		ndim = &vmodes[st.st_vmode].vm_ndim[GEOM_CUBE];
		dim.fv_w = cldim.fv_w + ndim->fv_w + 2 * SKEL_GAP;
		dim.fv_h = cldim.fv_h + ndim->fv_h + 2 * SKEL_GAP;
		dim.fv_z = cldim.fv_d + ndim->fv_d + 2 * SKEL_GAP;

		glPushMatrix();
		glTranslatef(
		    st.st_winsp.iv_w * st.st_wioff.iv_x - SKEL_GAP,
		    st.st_winsp.iv_h * st.st_wioff.iv_y - SKEL_GAP,
		    st.st_winsp.iv_d * st.st_wioff.iv_z - SKEL_GAP);
		draw_cube(&dim, &fill_clskel, 0);
		glPopMatrix();
		break;
	}
}

void
draw_pipe(struct ivec *iv, int dim)
{
	struct fill *fp;
	struct fvec *dimp;

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	fp = &fill_dim[dim];
	dimp = &vmodes[st.st_vmode].vm_ndim[GEOM_CUBE];

	glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
	glPushMatrix();
	glTranslatef(
	    st.st_winsp.iv_x * (st.st_wioff.iv_x + iv->iv_x - (dim == DIM_X)) + dimp->fv_w / 2.0,
	    st.st_winsp.iv_y * (st.st_wioff.iv_y + iv->iv_y - (dim == DIM_Y)) + dimp->fv_h / 2.0,
	    st.st_winsp.iv_z * (st.st_wioff.iv_z + iv->iv_z - (dim == DIM_Z)) + dimp->fv_d / 2.0);
	switch (dim) {
	case DIM_X:
		glRotatef(90.0, 0.0, 1.0, 0.0);
		break;
	case DIM_Y:
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		break;
	}
	gluCylinder(quadric, 0.1, 0.1,
	    st.st_winsp.iv_val[dim] * (widim.iv_val[dim] + 1), 3, 1);
	glPopMatrix();

	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);
}

__inline void
draw_node_pipes(struct node *n, int is_sel)
{
	int rsign, rd, flip, dim, port, class;
	struct fvec cen, ngcen;
	struct node *ng;
	struct fill *fp;
	double len;

	gluQuadricDrawStyle(quadric, GLU_FILL);

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	node_center(n, &cen);

	dim = 0; /* gcc */
	fp = NULL; /* gcc */

	for (rd = 0; rd < NRD; rd++) {
		ng = node_neighbor(VM_WIRED, n, rd, &flip);

		rsign = 1;
		switch (rd) {
		case RD_NEGX:
			rsign = -1;
			/* FALLTHROUGH */
		case RD_POSX:
			dim = DIM_X;
			break;
		case RD_NEGY:
			rsign = -1;
			/* FALLTHROUGH */
		case RD_POSY:
			dim = DIM_Y;
			break;
		case RD_NEGZ:
			rsign = -1;
			/* FALLTHROUGH */
		case RD_POSZ:
			dim = DIM_Z;
			break;
		}

		switch (st.st_pipemode) {
		case PM_DIR:
			fp = &fill_dim[dim];
			break;
		case PM_RTE:
			port = rdir_to_rteport(rd);

			if (n->n_route.rt_err[port][st.st_rtetype] == 0)
				continue;
			switch (st.st_rtepset) {
			case RPS_POS:
				if (rsign < 0)
					continue;
				break;
			case RPS_NEG:
				if (rsign > 0)
					continue;
				break;
			}
			class = roundclass(n->n_route.rt_err[port][st.st_rtetype],
			    0, rt_max.rt_err[port][st.st_rtetype], NRTC);
			fp = &rtpipeclass[class].nc_fill;
			break;
		}

		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		switch (st.st_vmode) {
		case VM_WIRED:
		case VM_WIONE:
			len = st.st_winsp.iv_val[dim];
			if (flip) {
				/*
				 * If we're drawing selpipes, we may
				 * need to draw continuation arrows
				 * as the continued nodes themselves
				 * will not have their arrows drawn.
				 */
				if (is_sel && node_show(ng) &&
				    (ng->n_flags & NF_SELNODE) == 0) {
					/* Draw continuation arrow. */
					node_center(ng, &ngcen);

					if (rsign > 0)
						len *= ng->n_wiv.iv_val[dim] + 1;
					else
						len *= widim.iv_val[dim] -
						    ng->n_wiv.iv_val[dim];

					glPushMatrix();
					glTranslatef(ngcen.fv_x, ngcen.fv_y, ngcen.fv_z);
					switch (dim) {
					case DIM_X:
						glRotatef(90.0, 0.0, -rsign * 1.0, 0.0);
						break;
					case DIM_Y:
						glRotatef(-90.0, -rsign * 1.0, 0.0, 0.0);
						break;
					case DIM_Z:
						if (rsign > 0)
							glRotatef(180.0, 0.0, 1.0, 0.0);
						break;
					}

					gluCylinder(quadric, 0.1, 0.1, len, 3, 1);
					glTranslatef(0.0, 0.0, len);
					gluCylinder(quadric, 0.5, 0.001, 0.5, 3, 1);
					glPopMatrix();
				}

				/* Calculate length to end of cluster. */
				len = st.st_winsp.iv_val[dim];
				if (rsign < 0)
					len *= n->n_wiv.iv_val[dim] + 1;
				else
					len *= widim.iv_val[dim] -
					    n->n_wiv.iv_val[dim];

			} else
				len *= abs(ng->n_wiv.iv_val[dim] -
				    n->n_wiv.iv_val[dim]);

			glPushMatrix();
			glTranslatef(cen.fv_x, cen.fv_y, cen.fv_z);
			switch (dim) {
			case DIM_X:
				glRotatef(90.0, 0.0, rsign * 1.0, 0.0);
				break;
			case DIM_Y:
				glRotatef(-90.0, rsign * 1.0, 0.0, 0.0);
				break;
			case DIM_Z:
				if (rsign < 0)
					glRotatef(180.0, 0.0, 1.0, 0.0);
				break;
			}

			gluCylinder(quadric, 0.1, 0.1, len, 3, 1);
			if (flip) {
				glTranslatef(0.0, 0.0, len);
				gluCylinder(quadric, 0.5, 0.001, 0.5, 3, 1);
			}
			glPopMatrix();
			break;
		case VM_PHYS:
			node_center(ng, &ngcen);

			glLineWidth(5.0);
			glBegin(GL_LINES);
			glVertex3d(cen.fv_x, cen.fv_y, cen.fv_z);
			glVertex3d(ngcen.fv_x, ngcen.fv_y, ngcen.fv_z);
			glEnd();
			break;
		}
	}

	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);
}

__inline void
draw_physpipes(int selpipes)
{
	int class, port, rd, r, c, ngr, ngc, dim, neg;
	struct node *n, *ng, *gn, *ln;
	struct physcoord pc, ngpc;
	struct fvec s, d;
	struct fill *fp;
	struct ivec iv;
	double ox, oy, oz;

	glLineWidth(5.0);
	NODE_FOREACH(n, &iv) {
		if (n == NULL || !node_show(n) ||
		    !SELNODE_IF_NEEDED(n, selpipes))
			continue;

		for (rd = 0; rd < NRD; rd++) {
			neg = 0;
			switch (rd) {
			case RD_NEGX:
				neg = 1;
				/* FALLTHROUGH */
			case RD_POSX:
				dim = DIM_X;
				break;
			case RD_NEGY:
				neg = 1;
				/* FALLTHROUGH */
			case RD_POSY:
				dim = DIM_Y;
				break;
			case RD_NEGZ:
				neg = 1;
				/* FALLTHROUGH */
			case RD_POSZ:
				dim = DIM_Z;
				break;
			default:
				continue;
			}

			/*
			 * If there is a neighbor whose
			 * pipes will be drawn, skip out
			 * on half of ours to avoid dups.
			 */
			ng = node_neighbor(VM_WIRED, n, rd, NULL);

			fp = NULL; /* gcc */
			switch (st.st_pipemode) {
			case PM_DIR:
				if (neg && node_show(ng) &&
				    SELNODE_IF_NEEDED(ng, selpipes))
					continue;

				fp = &fill_dim[dim];
				break;
			case PM_RTE:
				ln = NULL; /* gcc */
				switch (st.st_rtepset) {
				case RPS_POS:
					if (neg)
						continue;
					break;
				case RPS_NEG:
					if (!neg)
						continue;
					break;
				}
				port = rdir_to_rteport(rd);
				if (n->n_route.rt_err[port][st.st_rtetype] == 0)
					continue;
				class = roundclass(n->n_route.rt_err[port][st.st_rtetype],
				    0, rt_max.rt_err[port][st.st_rtetype], NRTC);
				fp = &rtpipeclass[class].nc_fill;
				break;
			}
			if (neg) {
				ln = ng;
				gn = n;
			} else {
				ln = n;
				gn = ng;
			}

			node_center(ln, &s);
			node_center(gn, &d);
			node_physpos(ln, &pc);
			node_physpos(gn, &ngpc);
			node_getmodpos(pc.pc_n, &r, &c);
			node_getmodpos(ngpc.pc_n, &ngr, &ngc);

			glColor3f(fp->f_r, fp->f_g, fp->f_b);
			glBegin(GL_LINE_STRIP);
			switch (dim) {
			case DIM_X: {
				int modoff;

				modoff = (NMODS * 2) - (pc.pc_m +
				    (pc.pc_cb % 2) * NMODS);

				ox = NODEWIDTH / 3.0;
				oz = NODEDEPTH;
				oy = NODEHEIGHT / 3.0 *
				    modoff / (NMODS * 2);
				if (c == 0)
					oz *= -1.0;
				if (s.fv_x > d.fv_x)
					ox *= -1.0;
				if (abs(pc.pc_cb - ngpc.pc_cb) != 2)
					oy *= -1.0;
				if (pc.pc_cb == 0 || ngpc.pc_cb == NCABS - 1)
					oy *= -1.0;

				glVertex3d(s.fv_x + ox, s.fv_y + oy, s.fv_z);
				glVertex3d(s.fv_x + ox, s.fv_y + oy, s.fv_z + oz);
				glVertex3d(d.fv_x - ox, d.fv_y + oy, d.fv_z + oz);
				glVertex3d(d.fv_x - ox, d.fv_y + oy, d.fv_z);
				break;
			    }
			case DIM_Y:
				if (pc.pc_cg == ngpc.pc_cg) {
					oy = 0.0;
					if (pc.pc_m == 0 && r == ngr)
						oy = NODEHEIGHT / 4.0;
					glVertex3d(s.fv_x, s.fv_y + oy, s.fv_z);
					glVertex3d(d.fv_x, d.fv_y + oy, d.fv_z);
				} else {
					ox = MODSPACE / 3.0;

					if (abs(pc.pc_cg - ngpc.pc_cg) > 1)
						ox *= 2.0;
					oy = NODEHEIGHT / 3.0;
					if (s.fv_y > d.fv_y)
						oy *= -1.0;

					glVertex3d(s.fv_x,      s.fv_y + oy, s.fv_z);
					glVertex3d(s.fv_x + ox, s.fv_y + oy, s.fv_z);
					glVertex3d(d.fv_x + ox, d.fv_y - oy, d.fv_z);
					glVertex3d(d.fv_x,      d.fv_y - oy, d.fv_z);
				}
				break;
			case DIM_Z:
				if (pc.pc_r != ngpc.pc_r)
					break;
				glVertex3d(s.fv_x, s.fv_y, s.fv_z);
				glVertex3d(d.fv_x, d.fv_y, d.fv_z);
				break;
			}
			glEnd();
		}
	}
}

__inline void
draw_pipes(int selpipes)
{
	struct node *n;
	struct ivec iv;

	switch (st.st_vmode) {
	case VM_WIRED:
	case VM_WIONE:
		gluQuadricDrawStyle(quadric, GLU_FILL);

		/*
		 * As some of the wired-mode pipe drawing
		 * code is icky, cut it into two parts so
		 * an 'optimized' mode for all pipes can
		 * exist instead of drawing tons of little
		 * cylinders all over the place.
		 */
		switch (st.st_pipemode) {
		case PM_DIR:
			if (!selpipes && (st.st_opts & OP_SUBSET) == 0)
				break;
			/* FALLTHROUGH */
		case PM_RTE:
			NODE_FOREACH(n, &iv)
				if (n && node_show(n) &&
				    SELNODE_IF_NEEDED(n, selpipes))
					draw_node_pipes(n, 0);
			return;
		}

		ivec_set(&iv, 0, 0, 0);
		for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
			for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
				draw_pipe(&iv, DIM_Z);

		ivec_set(&iv, 0, 0, 0);
		for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
			for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
				draw_pipe(&iv, DIM_Y);

		ivec_set(&iv, 0, 0, 0);
		for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
			for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
				draw_pipe(&iv, DIM_X);
		break;
	case VM_PHYS:
		draw_physpipes(selpipes);
		break;
	}
}

__inline void
draw_cluster(void)
{
	struct ivec iv;
	struct node *n;

	switch (st.st_vmode) {
	case VM_WIRED:
		break;
	case VM_WIONE:
	case VM_PHYS:
		if (st.st_opts & OP_PIPES)
			draw_pipes(0);
		NODE_FOREACH(n, &iv)
			if (n && node_show(n))
				draw_node(n, 0);
		if (st.st_opts & OP_SKEL)
			draw_skel();
		break;
	}
}

__inline void
draw_selnodes(void)
{
	struct selnode *sn;

	switch (st.st_vmode) {
	case VM_WIRED:
		break;
	case VM_WIONE:
	case VM_PHYS:
		if (st.st_opts & OP_SELPIPES &&
		    (st.st_opts & OP_PIPES) == 0)
			draw_pipes(1);
		SLIST_FOREACH(sn, &selnodes, sn_next)
			draw_node(sn->sn_nodep, NDF_NOTWEEN);
		break;
	}
}

void
make_ground(void)
{
	if (dl_ground[wid])
		glDeleteLists(dl_ground[wid], 1);
	dl_ground[wid] = glGenLists(1);
	glNewList(dl_ground[wid], GL_COMPILE);
	draw_ground();
	glEndList();
}

void
make_cluster(void)
{
	if (dl_cluster[wid])
		glDeleteLists(dl_cluster[wid], 1);
	dl_cluster[wid] = glGenLists(1);
	glNewList(dl_cluster[wid], GL_COMPILE);
	draw_cluster();
	glEndList();
}

void
make_selnodes(void)
{
	selnode_clean = 0;
	if (dl_selnodes[wid])
		glDeleteLists(dl_selnodes[wid], 1);
	if (SLIST_EMPTY(&selnodes)) {
		dl_selnodes[wid] = 0;
		return;
	}

	dl_selnodes[wid] = glGenLists(1);
	glNewList(dl_selnodes[wid], GL_COMPILE);
	draw_selnodes();
	glEndList();
}

#define distance(start, end, offv)					\
	sqrt(SQUARE((start)->fv_x - ((end)->fv_x + (offv)->fv_x)) +	\
	     SQUARE((start)->fv_y - ((end)->fv_y + (offv)->fv_y)) +	\
	     SQUARE((start)->fv_z - ((end)->fv_z + (offv)->fv_z)))

__inline void
draw_scene(void)
{
	static struct node *n;
	static struct ivec iv;
	static struct fvec v;
	static double max;

	switch (st.st_vmode) {
	case VM_WIRED:
		max = MAX3(
		    vmodes[VM_WIRED].vm_ndim[GEOM_CUBE].fv_w,
		    vmodes[VM_WIRED].vm_ndim[GEOM_CUBE].fv_h,
		    vmodes[VM_WIRED].vm_ndim[GEOM_CUBE].fv_d);

		WIREP_FOREACH(&v) {
			glPushMatrix();
			glTranslatef(v.fv_x, v.fv_y, v.fv_z);
			if (st.st_opts & OP_PIPES)
				draw_pipes(0);
			else if (st.st_opts & OP_SELPIPES)
				draw_pipes(1);
			NODE_FOREACH(n, &iv)
				if (n && node_show(n) &&
				    distance(&st.st_v, n->n_v, &v) < clip - max)
					draw_node(n, NDF_NOTWEEN);
			if (st.st_opts & OP_SKEL)
				draw_skel();
			glPopMatrix();
		}
		break;
	default:
		if (dl_selnodes[wid])
			glCallList(dl_selnodes[wid]);
		glCallList(dl_cluster[wid]);
		break;
	}
	if (st.st_opts & OP_GROUND)
		glCallList(dl_ground[wid]);
	if (!TAILQ_EMPTY(&panels))
		draw_panels(wid);
	if (!SLIST_EMPTY(&marks))
		mark_draw();
	if (!SLIST_EMPTY(&lnsegs))
		lnseg_draw();
	if (st.st_opts & OP_CAPTION)
		draw_caption();
// job_drawlabels();
}
