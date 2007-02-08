/* $Id$ */

#include "mon.h"

#include <err.h>

#include "draw.h"
#include "fill.h"
#include "xmath.h"
#include "state.h"

void
draw_text(const char *buf, struct fvec *dim, struct fill *fp)
{
	double scale, sw, ch, cw;

	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_LINE_SMOOTH);

	if (dim->fv_w < 0.0)
		glRotatef(90.0, 0.0, 1.0, 0.0);
	else
		glRotatef(90.0, 0.0, -1.0, 0.0);

	cw = 300.0;
	ch = 400.0;

	sw = strlen(buf) * cw;
	if (sw * fabs(dim->fv_y / dim->fv_w) < ch)
		/* Box is too small horizontally, scale by Y. */
		scale = fabs(dim->fv_h) / ch;
	else
		/* Box is too small vertically, scale by X. */
		scale = fabs(dim->fv_w) / sw;
	glScaled(scale, scale, scale);
//	glTranslatef(
//	    -bl.fv_x - (tr.fv_x - bl.fv_x) / 2.0f,
//	    -bl.fv_y - (tr.fv_y - bl.fv_y) / 2.0f, 0.0);

	glColor3f(fp->f_r, fp->f_g, fp->f_b);
	for (; *buf != '\0'; buf++) {
		switch (*buf) {
		case '0':
			/*
			 *     (w/3,h)                  (2w/3, h)
			 *             ################
			 *            ##################
			 * (0,2h/3)  ###              ###  (w,2h/3)
			 *          ###                ###
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ##                  ##
			 *          ## (w/8,h/3)        ##
			 *          ### (w/3,h/8)      ###
			 *  (0,h/3)  ###    (w,h/8)   ###  (w,h/3)
			 *            ######### ########
			 *             ######## #######
			 *     (w/3,0)      (w/2,0)     (2w/3,0)
			 */
			 {
			GLdouble v[][3] = {
				{ cw / 3.0,		0.0,		0.0 },
				{ 0.0,			ch / 3.0,	0.0 },
				{ 0.0,			2.0 * ch / 3.0,	0.0 },
				{ cw / 3.0,		ch,		0.0 },
				{ 2.0 * cw / 3.0,	ch,		0.0 },
				{ cw,			2.0 * ch / 3.0,	0.0 },
				{ cw,			ch / 3.0,	0.0 },
				{ 2.0 * cw / 3.0,	0.0,		0.0 },

				{ 2.0 * cw / 3.0,	ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	ch / 3.0,	0.0 },
				{ 7.0 * cw / 8.0,	2.0 * ch / 3.0,	0.0 },
				{ 2.0 * cw / 3.0,	7.0 * ch / 8.0,	0.0 },
				{ cw / 3.0,		7.0 * ch / 8.0,	0.0 },
				{ cw / 8.0,		2.0 * ch / 3.0,	0.0 },
				{ cw / 8.0,		ch / 3.0,	0.0 },
				{ cw / 3.0,		ch / 8.0,	0.0 }
			};

GLUtesselator *t = gluNewTess();
gluTessCallback(t, GLU_BEGIN, glBegin);
gluTessCallback(t, GLU_VERTEX, glVertex3dv);
gluTessCallback(t, GLU_END, glEnd);
gluBeginPolygon(t);
gluNextContour(t, GLU_EXTERIOR);

int i = 0;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;

gluNextContour(t, GLU_INTERIOR);
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;

gluEndPolygon(t);

}
			break;
		case '1':
			/*
			 *           (3w/8,h)    (5w/8,h)
			 *                   ####
			 *                  #####
			 *                 ######
			 *                #######
			 *               ### ####
			 *  (2.5w/8,5h/8)    ####
			 *                   ####
			 *                   ####
			 *                   ####
			 *                   ####
			 *                   ####
			 *                   ####
			 *                   ####
			 *        (3w/8,h/8) #### (5w/3,h/8)
			 *   (w/3,h/8) ################ (2w/3,h/8)
			 *             ################
			 *      (w/3,0)                (2w/3,0)
			 *
			 *
			 */
			 {
			GLdouble v[][3] = {
				{ cw / 8.0,		0.0,		0.0 },
				{ 7.0 * cw / 8.0,	0.0,		0.0 },
				{ 7.0 * cw / 8.0,	ch / 8.0,	0.0 },
				{ 5.0 * cw / 8.0,	ch / 8.0,	0.0 },
				{ 5.0 * cw / 8.0,	ch,		0.0 },
				{ 3.0 * cw / 8.0,	ch,		0.0 },
				{ cw / 8.0,	3.0 * ch / 4.0,	0.0 },
				{ 2.5 * cw / 8.0,	3.0 * ch / 4.0,	0.0 },
				{ 3.0 * cw / 8.0,	7.0 * ch / 8.0,	0.0 },
				{ 3.0 * cw / 8.0,	ch / 8.0,	0.0 },
				{ cw / 8.0,		ch / 8.0,	0.0 }

			};

GLUtesselator *t = gluNewTess();
gluTessCallback(t, GLU_BEGIN, glBegin);
gluTessCallback(t, GLU_VERTEX, glVertex3dv);
gluTessCallback(t, GLU_END, glEnd);
gluBeginPolygon(t);
gluNextContour(t, GLU_EXTERIOR);

int i = 0;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;
gluTessVertex(t, v[i], v[i]); i++;

gluEndPolygon(t);

}

			break;
		case '2':
			break;
		case '3':
			break;
		case '4':
			break;
		case '5':
			break;
		case '6':
			break;
		case '7':
			break;
		case '8':
			break;
		case '9':
			break;
		}
//		glEnd();
		glTranslatef(cw, 0.0, 0.0);
	}

//	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);

#if 0
 #####   #####  #       #######  #####  #######  #####   #####
#     # #     # #    #  #       #     # #    #  #     # #     #
      #       # #    #  #       #           #   #     # #     #
 #####   #####  #    #  ######  ######     #     #####   ######
#             # #######       # #     #   #     #     #       #
#       #     #      #  #     # #     #   #     #     # #     #
#######  #####       #   #####   #####    #      #####   #####
#endif

}















