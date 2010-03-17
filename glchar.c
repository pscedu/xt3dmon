/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

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
	glTranslatef(0.0, dim->fv_h / 2.0f, 0.0);
	glScaled(scale, scale, scale);
	glTranslatef(0.0, -ch / 2.0f, 0.0);

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
for (i = 0; i < sizeof(v) / sizeof(v[0]); i++)
gluTessVertex(t, v[i], v[i]); i++;

gluEndPolygon(t);

}

			break;
		case '2':
			/*
			 *       (w/8,h)             (7w/8,h)
			 *               ###########
			 *             ##############
			 *             ###(w/8,78)#### (w,7h/8)
			 *    (0,7h/8)    (w/8,3/4)###
			 *                         ###
			 *                         ###
			 *   (w/8,3.5h/8)   (7w8,) ### (w,3.5h/8)
			 *               ############
			 *              ############ (7w/8,h/3)
			 *    (0,h/3)  ### (w/8,h/3)
			 *             ###
			 *             ### (w/8,h/8)  (w,h/8)
			 *             ###############
			 *             ###############
			 *      (0,0)                  (w,0)
			 */
			 {
			GLdouble v[][3] = {
				{ 0.0,			0.0,		0.0 },
				{ cw,			0.0,		0.0 },
				{ cw,			ch / 8.0,	0.0 },
				{ cw / 8.0,		ch / 8.0,	0.0 },
				{ cw / 8.0,		ch / 3.0,	0.0 },
				{ 7.0 * cw / 8.0,	ch / 3.0,	0.0 },
				{ cw,			3.5 * ch / 8.0,	0.0 },
				{ cw,			7.0 * ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	ch,		0.0 },
				{ cw / 8.0,		ch,		0.0 },
				{ 0.0,			7.0 * ch / 8.0,	0.0 },
				{ cw / 8.0,		3.0 * ch / 4.0,	0.0 },
				{ cw / 3.0,		7.0 * ch / 8.0,	0.0 },
				{ 2.0 * cw / 3.0,	7.0 * ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	3.0 * ch / 4.0,	0.0 },
				{ 7.0 * cw / 8.0,	3.5 * ch / 8.0,	0.0 },
				{ cw / 8.0,		3.5 * ch / 8.0,	0.0 },
				{ 0.0,			ch / 3.0,	0.0 }
			};

GLUtesselator *t = gluNewTess();
gluTessCallback(t, GLU_BEGIN, glBegin);
gluTessCallback(t, GLU_VERTEX, glVertex3dv);
gluTessCallback(t, GLU_END, glEnd);
gluBeginPolygon(t);
gluNextContour(t, GLU_EXTERIOR);

int i = 0;
for (i = 0; i < sizeof(v) / sizeof(v[0]); i++)
gluTessVertex(t, v[i], v[i]); i++;

gluEndPolygon(t);

}
			break;
		case '3':
			/*
			 *        (w/8,h)            (7w/8,h)
			 *                ##########
			 *              #############
			 *             #####(13(23#### (w,7w/8)
			 *     (0,7h/8) ###        ###
			 *               (.5w/8)   ###
			 *                         ###
			 *                        #### (w,5h/8)
			 *                  #########
			 *                 ######### (7w/8,h/2)
			 *                  #########
			 *                        #### (w,3h/8)
			 *                         ###
			 *                         ###
			 *      (0,     ###        ###
			 *             #####      #### (w,h/8)
			 *              #############
			 *                ##########
			 *        (w/8,0)            (7w/8,0)
			 */
			 {
			GLdouble v[][3] = {
				{ cw / 8.0,		0.0,		0.0 },
				{ 7.0 * cw / 8.0,	0.0,		0.0 },
				{ cw,			ch / 8.0,	0.0 },
				{ cw,			3.0 * ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	ch / 2.0,	0.0 },
				{ cw,			5.0 * ch / 8.0,	0.0 },
				{ cw,			7.0 * ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	ch,		0.0 },
				{ cw / 8.0,		ch,		0.0 },
				{ 0.0,			7.0 * ch / 8.0,	0.0 },
				{ 0.5 * cw / 8.0,	3.0 * ch / 4.0,	0.0 },
				{ cw / 3.0,		7.0 * ch / 8.0,	0.0 },
				{ 2.0 * cw / 3.0,	7.0 * ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	3.0 * ch / 4.0,	0.0 },
				{ 3.0 * cw / 4.0,	5.0 * ch / 8.0,	0.0 },
				{ cw / 2.0,		4.5 * ch / 8.0,	0.0 },
				{ cw / 3.0,		ch / 2.0,	0.0 },
				{ cw / 2.0,		3.5 * ch / 8.0,	0.0 },
				{ 3.0 * cw / 4.0,	3.0 * ch / 8.0,	0.0 },
				{ 7.0 * cw / 8.0,	ch / 4.0,	0.0 },
				{ 2.0 * cw / 3.0,	ch / 8.0,	0.0 },
				{ cw / 3.0,		ch / 8.0,	0.0 },
				{ 0.5 * cw / 8.0,	ch / 4.0,	0.0 },
				{ 0.0,			ch / 8.0,	0.0 },
			};

GLUtesselator *t = gluNewTess();
gluTessCallback(t, GLU_BEGIN, glBegin);
gluTessCallback(t, GLU_VERTEX, glVertex3dv);
gluTessCallback(t, GLU_END, glEnd);
gluBeginPolygon(t);
gluNextContour(t, GLU_EXTERIOR);

int i = 0;
for (i = 0; i < sizeof(v) / sizeof(v[0]); i++)
gluTessVertex(t, v[i], v[i]); i++;

gluEndPolygon(t);

}
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
#       #######  #####  #######  #####   #####
#    #  #       #     # #    #  #     # #     #
#    #  #       #           #   #     # #     #
#    #  ######  ######     #     #####   ######
#######       # #     #   #     #     #       #
     #  #     # #     #   #     #     # #     #
     #   #####   #####    #      #####   #####
#endif

}















