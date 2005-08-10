/* $Id$ */

#include "compat.h"

#include <math.h>

#include "cdefs.h"
#include "mon.h"

/*
 * Determines the texture coordinate by finding the ratio
 * between the pixel value given and the max of the three
 * possible dimensions.  This gives a section of a texture
 * without stretching it.
 */
#define NODE_TEXCOORD(x, max) (1 / (max / x))


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
draw_box_outline(const struct fvec *dim, const struct fill *fillp)
{
	float w = dim->fv_w, h = dim->fv_h, d = dim->fv_d;
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
draw_box_filled(const struct fvec *dim, const struct fill *fillp)
{
	float w = dim->fv_w, h = dim->fv_h, d = dim->fv_d;
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

void
draw_box_tex(const struct fvec *dim, const struct fill *fillp, GLenum param)
{
	float w = dim->fv_w;
	float h = dim->fv_h;
	float d = dim->fv_d;
	float tw, th, td;
	float color[4];

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, fillp->f_texid);

	float max = MAX3(w, h, d);
	tw = NODE_TEXCOORD(w, max);
	th = NODE_TEXCOORD(h, max);
	td = NODE_TEXCOORD(d, max);

	color[0] = fillp->f_r;
	color[1] = fillp->f_g;
	color[2] = fillp->f_b;
	color[3] = fillp->f_a;

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, param);

	if (st.st_opts & OP_BLEND)
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

	/* Polygon Offset */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 3.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBegin(GL_QUADS);

	/* Back */
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glTexCoord2f(0.0, th);
	glVertex3f(0.0, h, 0.0);
	glTexCoord2f(tw, th);
	glVertex3f(w, h, 0.0);
	glTexCoord2f(tw, 0.0);
	glVertex3f(w, 0.0, 0.0);

	/* Front */
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.0, 0.0, d);
	glTexCoord2f(0.0, th);
	glVertex3f(0.0, h, d);
	glTexCoord2f(tw, th);
	glVertex3f(w, h, d);
	glTexCoord2f(tw, 0.0);
	glVertex3f(w, 0.0, d);

	/* Right */
	glTexCoord2f(0.0, 0.0);
	glVertex3f(w, 0.0, 0.0);
	glTexCoord2f(0.0, td);
	glVertex3f(w, 0.0, d);
	glTexCoord2f(th, td);
	glVertex3f(w, h, d);
	glTexCoord2f(th, 0.0);
	glVertex3f(w, h, 0.0);

	/* Left */
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glTexCoord2f(0.0, td);
	glVertex3f(0.0, 0.0, d);
	glTexCoord2f(th, td);
	glVertex3f(0.0, h, d);
	glTexCoord2f(th, 0.0);
	glVertex3f(0.0, h, 0.0);

	/* Top */
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.0, h, 0.0);
	glTexCoord2f(0.0, td);
	glVertex3f(0.0, h, d);
	glTexCoord2f(tw, td);
	glVertex3f(w, h, d);
	glTexCoord2f(tw, 0.0);
	glVertex3f(w, h, 0.0);

	/* Bottom */
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glTexCoord2f(0.0, td);
	glVertex3f(0.0, 0.0, d);
	glTexCoord2f(tw, td);
	glVertex3f(w, 0.0, d);
	glTexCoord2f(tw, 0.0);
	glVertex3f(w, 0.0, 0.0);

	glEnd();

	/* Disable polygon offset */
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_TEXTURE_2D);
}

/*
 * The following two mathematical algorithms were created from
 * pseudocode found in "Fundamentals of Interactive Computer Graphics".
 */
void
rgb_to_hsv(struct fill *c)
{
	float r = c->f_r;
	float g = c->f_g;
	float b = c->f_b;
	float max, min, ran;
	float rc, gc, bc;

	max = MAX3(r, g, b);
	min = MIN3(r, g, b);
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
hsv_to_rgb(struct fill *c)
{
	float s = c->f_s;
	float h = c->f_h;
	float v = c->f_v;
	float f, p, q, t;
	int i;

	if (s == 0) {
		c->f_r = v;
		c->f_g = v;
		c->f_b = v;
	} else {
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

/* Create a contrasting color */
void
rgb_contrast(struct fill *c)
{
	rgb_to_hsv(c);

	/* Rotate 180 degrees */
	c->f_h -= 180;

	if (c->f_h < 0)
		c->f_h += 360;

	/* Sat should be [0.3-1.0] */
	if (c->f_s < MID(SAT_MAX, SAT_MIN))
		c->f_s = SAT_MAX;
	else
		c->f_s = SAT_MIN;

	/* Val should be [0.5-1.0] */
	if (c->f_v < MID(VAL_MAX, VAL_MIN))
		c->f_v = VAL_MAX;
	else
		c->f_v = VAL_MIN;

	rgb_to_hsv(c);
}
