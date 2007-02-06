/* $Id$ */

#include "mon.h"

#include <err.h>

#include "draw.h"
#include "fill.h"
#include "xmath.h"
#include "state.h"

#include "FTGL/FTGLTextureFont.h"

FTFont *f = new FTGLTextureFont("data/font.ttf");

void
draw_text(const char *buf, struct fvec *dim, struct fill *fp)
{
	struct fvec tr, bl;
	double scale;

	glEnable(GL_TEXTURE_2D);
	if (dim->fv_w < 0.0)
		glRotatef(90.0, 0.0, 1.0, 0.0);
	else
		glRotatef(90.0, 0.0, -1.0, 0.0);
	glColor3f(fp->f_r, fp->f_g, fp->f_b);
if (st.st_rf & RF_CLUSTER)
	f->FaceSize(2);
else
	f->FaceSize(12);

	f->BBox(buf, bl.fv_x, bl.fv_y, bl.fv_z,
	    tr.fv_x, tr.fv_y, tr.fv_z);
	if ((tr.fv_x - bl.fv_x) * fabs(dim->fv_y / dim->fv_w) < (tr.fv_y - bl.fv_y))
		/* Box is too small horizontally, scale by Y. */
		scale = fabs(dim->fv_h) / (tr.fv_y - bl.fv_y);
	else
		/* Box is too small vertically, scale by X. */
		scale = fabs(dim->fv_w) / (tr.fv_x - bl.fv_x) * 0.975;
printf("%.4f\n", bl.fv_x);
	glTranslatef(fabs(dim->fv_w) / 2.0, fabs(dim->fv_h) / 2.0, 0.0);
	glScaled(scale, scale, scale);
	glTranslatef(
	    -bl.fv_x - (tr.fv_x - bl.fv_x) / 2.0f,
	    -bl.fv_y - (tr.fv_y - bl.fv_y) / 2.0f, 0.0);
//	f->Depth(20);
	f->Render(buf);
	glDisable(GL_TEXTURE_2D);
}
