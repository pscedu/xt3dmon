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
#include "nodeclass.h"
#include "panel.h"
#include "phys.h"
#include "queue.h"
#include "route.h"
#include "selnode.h"
#include "state.h"
#include "string.h"
#include "tween.h"
#include "xmath.h"

#define SELNODE_GAP	(0.1f)
#define SKEL_GAP	(0.1f)

float	 snap_to_grid(float, float, float);

struct fvec wi_repstart;
struct fvec wi_repdim;
float clip;

GLUquadric *quadric;

int dl_cluster[2];
int dl_ground[2];
int dl_selnodes[2];

void
draw_compass(void)
{
	struct fvec fv, upadj, normv;

	cam_getspecvec(&fv, 150, winv.iv_h - 150);
	fv.fv_x *= 2.0;
	fv.fv_y *= 2.0;
	fv.fv_z *= 2.0;
	vec_addto(&st.st_v, &fv);

	vec_set(&normv, 0.0, 1.0, 0.0);
	vec_sub(&upadj, &normv, &st.st_uv);

	glPushMatrix();
//	glTranslatef(upadj.fv_x, 0.0, upadj.fv_z);

	gluQuadricDrawStyle(quadric, GLU_FILL);

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
	gluCylinder(quadric, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
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
	gluCylinder(quadric, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
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
	gluCylinder(quadric, CMP_ATHK, 0.0001, CMP_ALEN, 4, 1);
	glPopMatrix();

	glPopMatrix();

	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
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

#define FTX_TWIDTH	1598
#define FTX_THEIGHT	34
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
	double uscale;
	const char *s;
	double z, d, h, y;
	char c;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, fill_font.f_texid_a[wid]);

	uscale = FTX_TWIDTH / (double)FTX_CWIDTH;

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

		glTexCoord2d(c / uscale, 0.0);
		glVertex3d(0.0, y, z);
		glTexCoord2d(c / uscale, 1.0);
		glVertex3d(0.0, y + h, z);
		glTexCoord2d((c + 1) / uscale, 1.0);
		glVertex3d(0.0, y + h, z + d);
		glTexCoord2d((c + 1) / uscale, 0.0);
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
	struct fvec *fvp;

	GLenum param = GL_REPLACE;

	if (!node_show(n))
		return;

	fp = n->n_fillp;

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
			draw_box_tex(n->n_dimp, fp, param);
		else
			draw_box_filled(n->n_dimp, fp);
	}

	if (fp->f_a != 1.0f)
		glDisable(GL_BLEND);

	if (st.st_opts & OP_FRAMES) {
		float col;

		col = fill_wireframe->f_a;
		fill_wireframe->f_a = fp->f_a;
		draw_box_outline(n->n_dimp, fill_wireframe);
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

	if (st.st_opts & OP_FRAMES)
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

		fdim.fv_w = physdim_top->pd_size.fv_w - 2 * fv.fv_x;
		fdim.fv_h = -fv.fv_y / 2.0f;
		fdim.fv_d = 2 * physdim_top->pd_size.fv_d +
		    physdim_top->pd_space - 2 * fv.fv_z;

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

void
draw_cluster_pipe(struct ivec *iv, struct fvec *sv, struct fvec *dimv)
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
		gluCylinder(quadric, 0.1, 0.1, len, 3, 1);
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

			fp = &rtclass[class / 10].nc_fill;

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
	struct fvec *ndim, sv, dim;
	struct ivec iv;

	glPushMatrix();
	glTranslatef(v->fv_x, v->fv_y, v->fv_z);

	ndim = &vmodes[st.st_vmode].vm_ndim[GEOM_CUBE];

	sv.fv_x = ndim->fv_w / 2 + st.st_wioff.iv_x * st.st_winsp.iv_x;
	sv.fv_y = ndim->fv_h / 2 + st.st_wioff.iv_y * st.st_winsp.iv_y;
	sv.fv_z = ndim->fv_d / 2 + st.st_wioff.iv_z * st.st_winsp.iv_z;

	gluQuadricDrawStyle(quadric, GLU_FILL);

	vec_set(&dim, 0.0f, 0.0f, WIV_SDEPTH);
	for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
		for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
			draw_cluster_pipe(&iv, &sv, &dim);

	vec_set(&dim, 0.0f, WIV_SHEIGHT, 0.0f);
	for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
		for (iv.iv_x = 0; iv.iv_x < widim.iv_w; iv.iv_x++)
			draw_cluster_pipe(&iv, &sv, &dim);

	vec_set(&dim, WIV_SWIDTH, 0.0f, 0.0f);
	for (iv.iv_y = 0; iv.iv_y < widim.iv_h; iv.iv_y++)
		for (iv.iv_z = 0; iv.iv_z < widim.iv_d; iv.iv_z++)
			draw_cluster_pipe(&iv, &sv, &dim);

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
			glPushMatrix();
			glTranslatef(n->n_v->fv_x, n->n_v->fv_y, n->n_v->fv_z);
			draw_node(n, NDF_DONTPUSH | NDF_NOOPTS);
			glPopMatrix();

			if (st.st_vmode != VM_PHYSICAL &&
			    st.st_opts & OP_SELPIPES &&
			    (st.st_opts & OP_PIPES) == 0)
				draw_node_pipes(n);
			break;
		}
		n->n_fillp = ofp;
	}

	glEndList();
}
