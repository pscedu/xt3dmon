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

	/* Bottom */
	glVertex3d(x,     y,     z);
	glVertex3d(x,     y,     z + d);
	glVertex3d(x + w, y,     z + d);
	glVertex3d(x + w, y,     z);
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

	if (st.st_opts & OP_FANCY) {
		GLfloat ambientLight[]  = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat diffuseLight[]  = { 0.2f, 0.2f, 0.2f, 1.0f };
		GLfloat specularLight[] = { 0.4f, 0.4f, 0.4f, 1.0f };
		GLfloat position0[] = {             -10.0f,             -10.0f,             -10.0f, 1.0f };
		GLfloat position1[] = { dimp->fv_w + 10.0f,             -10.0f,             -10.0f, 1.0f };
		GLfloat position2[] = {             -10.0f, dimp->fv_h + 10.0f,             -10.0f, 1.0f };
		GLfloat position3[] = {             -10.0f,             -10.0f, dimp->fv_d + 10.0f, 1.0f };
		GLfloat position4[] = { dimp->fv_w + 10.0f, dimp->fv_h + 10.0f,             -10.0f, 1.0f };
		GLfloat position5[] = { dimp->fv_w + 10.0f,             -10.0f, dimp->fv_d + 10.0f, 1.0f };
		GLfloat position6[] = {             -10.0f, dimp->fv_h + 10.0f, dimp->fv_d + 10.0f, 1.0f };
		GLfloat position7[] = { dimp->fv_w + 10.0f, dimp->fv_h + 10.0f, dimp->fv_d + 10.0f, 1.0f };

#define SQRT3 (0.577350269189626)
		GLfloat look0[] = {  SQRT3,  SQRT3,  SQRT3, 1.0f };
		GLfloat look1[] = { -SQRT3,  SQRT3,  SQRT3, 1.0f };
		GLfloat look2[] = {  SQRT3, -SQRT3,  SQRT3, 1.0f };
		GLfloat look3[] = {  SQRT3,  SQRT3, -SQRT3, 1.0f };
		GLfloat look4[] = { -SQRT3, -SQRT3,  SQRT3, 1.0f };
		GLfloat look5[] = { -SQRT3,  SQRT3, -SQRT3, 1.0f };
		GLfloat look6[] = {  SQRT3, -SQRT3, -SQRT3, 1.0f };
		GLfloat look7[] = { -SQRT3, -SQRT3, -SQRT3, 1.0f };

		float materialLight[] = { 0.8f, 0.8f, 0.8f, 1.0f };
		{ struct fill f = *fp;

		materialLight[0] = f.f_r * 0.8;
		materialLight[1] = f.f_g * 0.8;
		materialLight[2] = f.f_b * 0.8;

//		fill_contrast(&f);
		specularLight[0] = f.f_r * 1.1;
		specularLight[1] = f.f_g * 1.1;
		specularLight[2] = f.f_b * 1.1;
		}

		int param = GL_AMBIENT_AND_DIFFUSE;

		if (fp->f_r > .8 || fp->f_g > .8 || fp->f_b > .8)
			param = GL_SPECULAR;

		glFrontFace(GL_CCW);
		glColorMaterial(GL_FRONT_AND_BACK, param);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
//		glEnable(GL_LIGHT0);		/* ? - - */
//		glEnable(GL_LIGHT1);		/* ? - - */
//		glEnable(GL_LIGHT2);		/* ? - - */
		glEnable(GL_LIGHT3);		/* + + + */
//		glEnable(GL_LIGHT4);		/* ? - - */
		glEnable(GL_LIGHT5);		/* - + + */
		glEnable(GL_LIGHT6);		/* + - + */
		glEnable(GL_LIGHT7);		/* - - + */
		glEnable(GL_NORMALIZE);

		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, materialLight);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularLight);

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT0, GL_POSITION, position0);
		glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, look0);
		glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT1, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT1, GL_POSITION, position1);
		glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, look1);
		glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT2, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT2, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT2, GL_POSITION, position2);
		glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, look2);
		glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT3, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT3, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT3, GL_POSITION, position3);
		glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, look3);
		glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT4, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT4, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT4, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT4, GL_POSITION, position4);
		glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, look4);
		glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT5, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT5, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT5, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT5, GL_POSITION, position5);
		glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, look5);
		glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT6, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT6, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT6, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT6, GL_POSITION, position6);
		glLightfv(GL_LIGHT6, GL_SPOT_DIRECTION, look6);
		glLightf(GL_LIGHT6, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT6, GL_LINEAR_ATTENUATION, 0.05);

		glLightfv(GL_LIGHT7, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT7, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT7, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT7, GL_POSITION, position7);
		glLightfv(GL_LIGHT7, GL_SPOT_DIRECTION, look7);
		glLightf(GL_LIGHT7, GL_CONSTANT_ATTENUATION, 0.1);
		glLightf(GL_LIGHT7, GL_LINEAR_ATTENUATION, 0.05);

		glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 100);

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

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
		    fp->f_a == 1.0f ? GL_REPLACE : GL_BLEND);

		if (fp->f_a != 1.0f)
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,
			    fp->f_rgba);

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

	if (st.st_opts & OP_FANCY) {
		glDisable(GL_NORMALIZE);
		glDisable(GL_LIGHT7);
		glDisable(GL_LIGHT6);
		glDisable(GL_LIGHT5);
		glDisable(GL_LIGHT4);
		glDisable(GL_LIGHT3);
		glDisable(GL_LIGHT2);
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT0);
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHTING);
	}
	if (flags & DF_FRAME) {
		/* Anti-aliasing */
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE); // GL_NICEST

		if (fp->f_flags & FF_SKEL)
			glLineWidth(1.6f);
		else
			glLineWidth(0.6f);
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
#define SPH_CUTS_FANCY 30

/*
 * Draw a sphere at (x,y,z) with center at (x+r,y+r,z+r).
 */
void
draw_sphere(const struct fvec *dimp, const struct fill *fp, int flags)
{
	struct fill f_frame;
	int ldflags, cuts;
	double r;

	if (st.st_opts & OP_FANCY)
		cuts = SPH_CUTS_FANCY;
	else
		cuts = SPH_CUTS;

	ldflags = 0;
	f_frame = *fp;
	r = dimp->fv_r;

	glPushMatrix();
	glTranslatef(dimp->fv_r, dimp->fv_r, dimp->fv_r);

	if ((fp->f_flags & FF_SKEL) == 0) {
		if (fp->f_a != 1.0) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, fp->f_blfunc ?
			    fp->f_blfunc : GL_DST_COLOR);
		}

//		glEnable(GL_POLYGON_SMOOTH);
//		glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

		gluQuadricDrawStyle(quadric, GLU_FILL);
		glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		gluSphere(quadric, r, cuts, cuts);

//		glDisable(GL_POLYGON_SMOOTH);

		if (fp->f_a != 1.0)
			glDisable(GL_BLEND);

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

		glLineWidth(0.6f);
		gluQuadricDrawStyle(quadric, GLU_SILHOUETTE);
		glColor4f(f_frame.f_r, f_frame.f_g, f_frame.f_b,
		    f_frame.f_a);
		gluSphere(quadric, r, cuts, cuts);

		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}
