/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>

#include "cdefs.h"
#include "draw.h"
#include "env.h"
#include "fill.h"
#include "state.h"
#include "xmath.h"

#define SHIFT_OFFSET (0.001)

#define LDF_SHIFT (1<<0)

__inline void
draw_cube_filled(const struct fvec *dimp, int flags)
{
	double w = dimp->fv_w;
	double h = dimp->fv_h;
	double d = dimp->fv_d;

	double x = 0.0;
	double y = 0.0;
	double z = 0.0;

	if (flags & LDF_SHIFT) {
		x -= SHIFT_OFFSET;
		y -= SHIFT_OFFSET;
		z -= SHIFT_OFFSET;

		w += 2.0 * SHIFT_OFFSET;
		h += 2.0 * SHIFT_OFFSET;
		d += 2.0 * SHIFT_OFFSET;
	}

	/* Back */
	glVertex3d(x,     y,     z);
	glVertex3d(x,     y + h, z);
	glVertex3d(x + w, y + h, z);
	glVertex3d(x + w, y,     z);

	/* Front */
	glVertex3d(x + w, y,     z + d);
	glVertex3d(x + w, y + h, z + d);
	glVertex3d(x,     y + h, z + d);
	glVertex3d(x,     y,     z + d);

	/* Right */
	glVertex3d(x + w, y,     z + d);
	glVertex3d(x + w, y + h, z + d);
	glVertex3d(x + w, y + h, z);
	glVertex3d(x + w, y,     z);

	/* Left */
	glVertex3d(x,     y,     z);
	glVertex3d(x,     y,     z + d);
	glVertex3d(x,     y + h, z + d);
	glVertex3d(x,     y + h, z);

	/* Top */
	glVertex3d(x,     y + h, z + d);
	glVertex3d(x + w, y + h, z + d);
	glVertex3d(x + w, y + h, z);
	glVertex3d(x,     y + h, z);

#if 0
	/* Bottom */
	glVertex3d(x,     y,     z);
	glVertex3d(x,     y,     z + d);
	glVertex3d(x + w, y,     z + d);
	glVertex3d(x + w, y,     z);
#endif
}

__inline void
draw_cube_tex(const struct fvec *dim, const struct fvec *texv)
{
	double tw = texv->fv_w;
	double th = texv->fv_h;
	double td = texv->fv_d;

	double w = dim->fv_w;
	double h = dim->fv_h;
	double d = dim->fv_d;

	/* Back */
	glTexCoord2d(0.0, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
	glTexCoord2d(0.0, th);
	glVertex3d(0.0, h, 0.0);
	glTexCoord2d(tw, th);
	glVertex3d(w, h, 0.0);
	glTexCoord2d(tw, 0.0);
	glVertex3d(w, 0.0, 0.0);

	/* Front */
	glTexCoord2d(0.0, 0.0);
	glVertex3d(0.0, 0.0, d);
	glTexCoord2d(0.0, th);
	glVertex3d(0.0, h, d);
	glTexCoord2d(tw, th);
	glVertex3d(w, h, d);
	glTexCoord2d(tw, 0.0);
	glVertex3d(w, 0.0, d);

	/* Right */
	glTexCoord2d(0.0, 0.0);
	glVertex3d(w, 0.0, 0.0);
	glTexCoord2d(0.0, td);
	glVertex3d(w, 0.0, d);
	glTexCoord2d(th, td);
	glVertex3d(w, h, d);
	glTexCoord2d(th, 0.0);
	glVertex3d(w, h, 0.0);

	/* Left */
	glTexCoord2d(0.0, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
	glTexCoord2d(0.0, td);
	glVertex3d(0.0, 0.0, d);
	glTexCoord2d(th, td);
	glVertex3d(0.0, h, d);
	glTexCoord2d(th, 0.0);
	glVertex3d(0.0, h, 0.0);

	/* Top */
	glTexCoord2d(0.0, 0.0);
	glVertex3d(0.0, h, 0.0);
	glTexCoord2d(0.0, td);
	glVertex3d(0.0, h, d);
	glTexCoord2d(tw, td);
	glVertex3d(w, h, d);
	glTexCoord2d(tw, 0.0);
	glVertex3d(w, h, 0.0);

	/* Bottom */
	glTexCoord2d(0.0, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
	glTexCoord2d(0.0, td);
	glVertex3d(0.0, 0.0, d);
	glTexCoord2d(tw, td);
	glVertex3d(w, 0.0, d);
	glTexCoord2d(tw, 0.0);
	glVertex3d(w, 0.0, 0.0);
}

void
draw_cube(const struct fvec *dimp, const struct fill *fp, int flags)
{
	struct fill f_frame;
	struct fvec texv;
	float color[4];
	double max;
	int ldflags;

	ldflags = 0;
	f_frame = fill_black;
	f_frame.f_a = fp->f_a;

	if (fp->f_flags & FF_SKEL) {
		flags |= DF_FRAME;
		f_frame = *fp;
	} else {
		if (st.st_opts & OP_FRAMES)
			ldflags |= LDF_SHIFT;
		else
			flags &= ~DF_FRAME;
	}

	if (fp->f_a != 1.0f) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, fp->f_blfunc ?
		    fp->f_blfunc : GL_DST_COLOR);
	}

	if (fp->f_flags & FF_TEX) {
		glEnable(GL_TEXTURE_2D);

		if (fp->f_a == 1.0f)
			glBindTexture(GL_TEXTURE_2D, fp->f_texid[wid]);
		else
			glBindTexture(GL_TEXTURE_2D, fp->f_texid_a[wid]);

		max = MAX3(dimp->fv_w, dimp->fv_h, dimp->fv_d);
		texv.fv_w = TEXCOORD(dimp->fv_w, max);
		texv.fv_h = TEXCOORD(dimp->fv_h, max);
		texv.fv_d = TEXCOORD(dimp->fv_d, max);

		color[0] = fp->f_r;
		color[1] = fp->f_g;
		color[2] = fp->f_b;
		color[3] = fp->f_a;

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
		    fp->f_a == 1.0f ? GL_REPLACE : GL_BLEND);

		if (fp->f_a != 1.0f)
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 3.0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBegin(GL_QUADS);
		draw_cube_tex(dimp, &texv);
		glEnd();

		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_POLYGON_OFFSET_FILL);
	//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDisable(GL_TEXTURE_2D);
	} else if ((fp->f_flags & DF_FRAME) == 0) {
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		glBegin(GL_QUADS);
		draw_cube_filled(dimp, 0);
		glEnd();
	}

	if (fp->f_a != 1.0f)
		glDisable(GL_BLEND);

	if (flags & DF_FRAME) {
		/* Anti-aliasing */
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

		glLineWidth(0.1f);
		glColor4f(f_frame.f_r, f_frame.f_g, f_frame.f_b,
		    f_frame.f_a);
		glBegin(GL_LINE_STRIP);
		draw_cube_filled(dimp, ldflags);
		glEnd();

		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
	}
}

#define SPH_CUTS 7

/*
 * Draw a sphere at (x,y,z) with center at (x+r,y+r,z+r).
 */
void
draw_sphere(const struct fvec *dimp, const struct fill *fp, int flags)
{
	struct fill f_frame;
	int ldflags;
	double r;

	ldflags = 0;
	f_frame = *fp;
	r = dimp->fv_r;

	glPushMatrix();
	glTranslatef(dimp->fv_r, dimp->fv_r, dimp->fv_r);

	if ((fp->f_flags & FF_SKEL) == 0) {
//		glEnable(GL_POLYGON_SMOOTH);
//		glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

		gluQuadricDrawStyle(quadric, GLU_FILL);
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		gluSphere(quadric, r, SPH_CUTS, SPH_CUTS);

//		glDisable(GL_POLYGON_SMOOTH);

		ldflags |= LDF_SHIFT;
		f_frame = fill_black;
		f_frame.f_a = 0.2f;
	}

	if (fp->f_flags & FF_SKEL ||
	    (flags & DF_FRAME && st.st_opts & OP_FRAMES)) {
		glEnable(GL_BLEND);
		glEnable(GL_LINE_SMOOTH);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

		if (ldflags & LDF_SHIFT) {
			r += SHIFT_OFFSET;
			glTranslatef(SHIFT_OFFSET, SHIFT_OFFSET, SHIFT_OFFSET);
		}

		glLineWidth(0.1f);
		gluQuadricDrawStyle(quadric, GLU_SILHOUETTE);
		glColor4f(f_frame.f_r, f_frame.f_g, f_frame.f_b,
		    f_frame.f_a);
		gluSphere(quadric, r, SPH_CUTS, SPH_CUTS);

		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}
