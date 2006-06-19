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
#include "xmath.h"

#define SKEL_GAP	(0.1f)

float	 snap_to_grid(float, float, float);

struct fvec wi_repstart;
struct fvec wi_repdim;
float clip;

GLUquadric *quadric;

int dl_cluster[2];
int dl_ground[2];
int dl_selnodes[2];

__inline void
draw_compass(int u, __unused int w, int v, __unused int h)
{
	double cl, mvm[16], pvm[16], x, y, z;
	struct fvec fv;
	int vp[4];

	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
	glGetDoublev(GL_PROJECTION_MATRIX, pvm);

	if (gluUnProject(u, v, 0, mvm, pvm, vp, &x, &y, &z) == GL_FALSE)
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
	glLineWidth(0.6);
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	cl = 0.05;
#define CMP_ATHK (0.005)
#define CMP_ALEN (0.01)

	glColor3f(0.0f, 1.0f, 0.0f);					/* z - green */
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

	glColor3f(1.0f, 0.0f, 0.0f);					/* y - red */
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

	glColor3f(0.0f, 0.0f, 1.0f);					/* x - blue */
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

	glPopMatrix();

	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

__inline void
draw_string(int u, int v, int h, void *font, const unsigned char *s)
{
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
	while (*s != '\0')
		glutBitmapCharacter(font, *s++);

	u++;
	v--;
	glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
	glRasterPos2f(u, v);
	while (*s != '\0')
		glutBitmapCharacter(font, *s++);

	margin = 2;
	u -= 1 + margin;
	w += margin * 2;
	h += margin * 2;
	v += h - 4 - margin;

	glColor4f(0.2f, 0.2f, 0.2f, 0.6f);
	glBegin(GL_POLYGON);
	glVertex2d(u,		v);
	glVertex2d(u + w,	v);
	glVertex2d(u + w - 1,	v - h + 1);
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
	extern char *caption;
	const unsigned char *s;
	void *font;
	int u, w, h;

	font = GLUT_BITMAP_TIMES_ROMAN_24;
	h = 24;

	/* Draw the caption text, center on x */
	if (caption) {
		s = (const unsigned char *)caption;
		w = glutBitmapLength(font, s);
		u = (winv.iv_w / 2.0) - (w / 2.0);
		draw_string(u, 10, h, font, s);
	}
	s = (const unsigned char *)"Live BigBen/XT3 Monitor";
	w = glutBitmapLength(font, s);
	u = (winv.iv_w / 2.0) - (w / 2.0);
	draw_string(u, winv.iv_h - h - 3, h, font, s);
}

__inline void
draw_scene(void)
{
	static struct fvec v;

	switch (st.st_vmode) {
	case VM_WIRED:
		WIREP_FOREACH(&v) {
			glPushMatrix();
			glTranslatef(v.fv_x, v.fv_y, v.fv_z);
			if (dl_selnodes[wid])
				glCallList(dl_selnodes[wid]);
			glCallList(dl_cluster[wid]);
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
draw_node_label(struct node *n)
{
	char buf[BUFSIZ];
	struct fvec dim;
	struct fill c;

	c = *n->n_fillp;
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
 * Special case of pipe-drawing code: draw pipes around selected node
 * only.
 *
 * XXX: respect st_pipemode.
 */
__inline void
draw_node_pipes(struct node *np)
{
	gluQuadricDrawStyle(quadric, GLU_FILL);

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

	glColor4f(0.0f, 1.0f, 0.0f, 0.7f);				/* z - green */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + np->n_dimp->fv_w / 2.0,
	    np->n_v->fv_y + np->n_dimp->fv_h / 2.0,
	    np->n_v->fv_z + np->n_dimp->fv_d / 2.0 - st.st_winsp.iv_d);
	gluCylinder(quadric, 0.1, 0.1, 2.0 * st.st_winsp.iv_d, 3, 1);
	glPopMatrix();

	glColor4f(1.0f, 0.0f, 0.0f, 0.7f);				/* y - red */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + np->n_dimp->fv_w / 2.0,
	    np->n_v->fv_y + np->n_dimp->fv_h / 2.0 - st.st_winsp.iv_h,
	    np->n_v->fv_z + np->n_dimp->fv_d / 2.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(quadric, 0.1, 0.1, 2.0 * st.st_winsp.iv_h, 3, 1);
	glPopMatrix();

	glColor4f(0.0f, 0.0f, 1.0f, 0.7f);				/* x - blue */
	glPushMatrix();
	glTranslatef(
	    np->n_v->fv_x + np->n_dimp->fv_w / 2.0 - st.st_winsp.iv_w,
	    np->n_v->fv_y + np->n_dimp->fv_h / 2.0,
	    np->n_v->fv_z + np->n_dimp->fv_d / 2.0);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	gluCylinder(quadric, 0.1, 0.1, 2.0 * st.st_winsp.iv_w, 3, 1);
	glPopMatrix();

	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_BLEND);
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

//	if (!node_show(n))
//		return;

	if ((flags & NDF_ATORIGIN) == 0) {
		if (st.st_opts & OP_NODEANIM &&
		    (node_tween_dir(&n->n_vcur.fv_x, &n->n_v->fv_x) |
		    node_tween_dir(&n->n_vcur.fv_y, &n->n_v->fv_y) |
		    node_tween_dir(&n->n_vcur.fv_z, &n->n_v->fv_z))) {
			st.st_rf |= RF_CLUSTER | RF_SELNODE;
			fvp = &n->n_vcur;
		} else
			fvp = n->n_v;
		glPushMatrix();
		glTranslatef(fvp->fv_x, fvp->fv_y, fvp->fv_z);
	}

	switch (n->n_geom) {
	case GEOM_CUBE:
		draw_cube(n->n_dimp, n->n_fillp, DF_FRAME);
		break;
	case GEOM_SPHERE:
		draw_sphere(n->n_dimp, n->n_fillp, DF_FRAME);
		break;
	}

	if (st.st_opts & OP_NLABELS)
		draw_node_label(n);

	if ((flags & NDF_ATORIGIN) == 0)
		glPopMatrix();
}

__inline void
draw_ground(void)
{
	struct fvec fv, fdim, *ndim;

	/* Anti-aliasing */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

	glLineWidth(1.0);
	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 1.0f);			/* blue */
	glVertex3f(-500.0f, 0.0f, 0.0f);		/* x-axis */
	glVertex3f(500.0f, 0.0f, 0.0f);

	glColor3f(1.0f, 0.0f, 0.0f);			/* red */
	glVertex3f(0.0f, -500.0f, 0.0f);		/* y-axis */
	glVertex3f(0.0f, 500.0f, 0.0f);

	glColor3f(0.0f, 1.0f, 0.0f);			/* green */
	glVertex3f(0.0f, 0.0f, -500.0f);		/* z-axis */
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
draw_cluster_pipe(struct ivec *iv, struct fvec *dimv)
{
	struct fill *fp, dims[] = {
		FILL_INITA(0.0f, 0.0f, 1.0f, 0.5f),	/* x - blue */
		FILL_INITA(1.0f, 0.0f, 0.0f, 0.5f),	/* y - red */
		FILL_INITA(0.0f, 1.0f, 0.0f, 0.5f)	/* z - green */
	};
	int *j, jmax, dim, port, class;
	struct ivec adjv, *spv;
	struct fvec sv, *np;
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

	sv.fv_x = vmodes[st.st_vmode].vm_ndim[GEOM_CUBE].fv_w / 2;
	sv.fv_y = vmodes[st.st_vmode].vm_ndim[GEOM_CUBE].fv_h / 2;
	sv.fv_z = vmodes[st.st_vmode].vm_ndim[GEOM_CUBE].fv_d / 2;

	switch (st.st_pipemode) {
	case PM_DIR:	/* color for torus directions */
		sv.fv_x += st.st_wioff.iv_x * st.st_winsp.iv_x;
		sv.fv_y += st.st_wioff.iv_y * st.st_winsp.iv_y;
		sv.fv_z += st.st_wioff.iv_z * st.st_winsp.iv_z;

		fp = &dims[dim];
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		glPushMatrix();

		switch (dim) {
		case DIM_X:
			glTranslatef(
			    sv.fv_x - spv->iv_x,
			    sv.fv_y + iv->iv_y * spv->iv_y,
			    sv.fv_z + iv->iv_z * spv->iv_z);
			glRotatef(90.0, 0.0, 1.0, 0.0);
			len = (widim.iv_w + 1) * st.st_winsp.iv_x;
			break;
		case DIM_Y:
			glTranslatef(
			    sv.fv_x + iv->iv_x * spv->iv_x,
			    sv.fv_y - spv->iv_y,
			    sv.fv_z + iv->iv_z * spv->iv_z);
			glRotatef(-90.0, 1.0, 0.0, 0.0);
			len = (widim.iv_h + 1) * st.st_winsp.iv_y;
			break;
		case DIM_Z:
			glTranslatef(
			    sv.fv_x + iv->iv_x * spv->iv_x,
			    sv.fv_y + iv->iv_y * spv->iv_y,
			    sv.fv_z - spv->iv_z);
			len = (widim.iv_d + 1) * st.st_winsp.iv_z;
			break;
		}
		gluCylinder(quadric, 0.1, 0.1, len, 3, 1);
		glPopMatrix();
		break;
	case PM_RT:	/* color for route errors */
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

		for (*j = 0; *j < jmax; ++*j) {
			adjv.iv_x = negmod(iv->iv_x + st.st_wioff.iv_x, widim.iv_w);
			adjv.iv_y = negmod(iv->iv_y + st.st_wioff.iv_y, widim.iv_h);
			adjv.iv_z = negmod(iv->iv_z + st.st_wioff.iv_z, widim.iv_d);
			n = wimap[adjv.iv_x][adjv.iv_y][adjv.iv_z];
			if (n == NULL || !node_show(n))
				continue;

			port = DIM_TO_PORT(dim, st.st_rtepset);
			if (n->n_route.rt_err[port][st.st_rtetype] == DV_NODATA)
				continue;

			if (rt_max.rt_err[port][st.st_rtetype])
				class = 100 * n->n_route.rt_err[port][st.st_rtetype] /
				    rt_max.rt_err[port][st.st_rtetype];
			else
				class = 100;		/* 100% */

			fp = &rtclass[class / 10].nc_fill;

			glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
			glPushMatrix();

			if (st.st_opts & OP_NODEANIM)
				np = &n->n_vcur;
			else
				np = n->n_v;

			switch (dim) {
			case DIM_X:
				glTranslatef(
				    sv.fv_x + np->fv_x - spv->iv_x,
				    sv.fv_y + np->fv_y,
				    sv.fv_z + np->fv_z);
				glRotatef(90.0, 0.0, 1.0, 0.0);
				break;
			case DIM_Y:
				glTranslatef(
				    sv.fv_x + np->fv_x,
				    sv.fv_y + np->fv_y - spv->iv_y,
				    sv.fv_z + np->fv_z);
				glRotatef(-90.0, 1.0, 0.0, 0.0);
				break;
			case DIM_Z:
				glTranslatef(
				    sv.fv_x + np->fv_x,
				    sv.fv_y + np->fv_y,
				    sv.fv_z + np->fv_z - spv->iv_z);
				break;
			}

			gluCylinder(quadric, 0.1, 0.1, len, 3, 1);
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
	struct fvec dim;
	struct ivec iv;

	glPushMatrix();
	glTranslatef(v->fv_x, v->fv_y, v->fv_z);
	gluQuadricDrawStyle(quadric, GLU_FILL);

	vec_set(&dim, 0.0f, 0.0f, WIV_SDEPTH);
	for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
		for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
			draw_cluster_pipe(&iv, &dim);

	vec_set(&dim, 0.0f, WIV_SHEIGHT, 0.0f);
	for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
		for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
			draw_cluster_pipe(&iv, &dim);

	vec_set(&dim, WIV_SWIDTH, 0.0f, 0.0f);
	for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
		for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
			draw_cluster_pipe(&iv, &dim);

	glPopMatrix();
}

__inline void
draw_cluster(void)
{
	struct ivec iv;
	struct node *n;
	int ndf;

	ndf = 0;
	switch (st.st_vmode) {
	case VM_WIRED:
		ndf |= NDF_ATORIGIN;
		/* FALLTHROUGH */
	case VM_WIONE:
		if (st.st_opts & OP_PIPES)
			draw_cluster_pipes(&fv_zero);
		/* FALLTHROUGH */
	case VM_PHYS:
		NODE_FOREACH(n, &iv)
			if (n && node_show(n)) {
				if (ndf & NDF_ATORIGIN) {
					glPushMatrix();
					glTranslatef(
					    n->n_v->fv_x,
					    n->n_v->fv_y,
					    n->n_v->fv_z);
				}
				draw_node(n, ndf);
				if (ndf & NDF_ATORIGIN)
					glPopMatrix();
			}
		break;
	}

	if (st.st_opts & OP_SKEL)
		draw_skel();
}

__inline void
draw_selnodes(void)
{
	struct selnode *sn;
	struct fvec *fvp;
	struct fill *ofp;
	struct node *n;

	SLIST_FOREACH(sn, &selnodes, sn_next) {
		n = sn->sn_nodep;
		ofp = n->n_fillp;
		n->n_fillp = &fill_selnode;

		if (st.st_opts & OP_NODEANIM)
			fvp = &n->n_vcur;
		else
			fvp = n->n_v;

		glPushMatrix();
		glTranslatef(fvp->fv_x, fvp->fv_y, fvp->fv_z);
		draw_node(n, NDF_ATORIGIN);
		glPopMatrix();

		if (st.st_vmode != VM_PHYS &&
		    st.st_opts & OP_SELPIPES &&
		    (st.st_opts & OP_PIPES) == 0)
			draw_node_pipes(n);

		n->n_fillp = ofp;
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
