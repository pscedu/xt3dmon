/* $Id$ */

#include "cdefs.h"
#include "env.h"
#include "fill.h"
#include "nodeclass.h"
#include "util.h"

struct fill fill_black		= FILL_INIT(0.0f, 0.0f, 0.0f);
struct fill fill_grey		= FILL_INIT(0.2f, 0.2f, 0.2f);
struct fill fill_light_blue	= FILL_INIT(0.2f, 0.4f, 0.6f);
struct fill fill_white		= FILL_INIT(1.0f, 1.0f, 1.0f);
struct fill fill_xparent	= FILL_INITFA(1.0f, 1.0f, 1.0f, 0.0f, FF_ALPHA);
struct fill fill_yellow		= FILL_INIT(1.0f, 1.0f, 0.0f);

struct fill fill_bg		= FILL_INIT(0.1f, 0.2f, 0.3f);
struct fill fill_font		= FILL_INIT(0.0f, 0.0f, 0.0f);
struct fill fill_nodata		= FILL_INITF(1.0f, 1.0f, 0.0f, FF_SKEL);
struct fill fill_same		= FILL_INIT(1.0f, 1.0f, 1.0f);
struct fill fill_selnode	= FILL_INIT(0.2f, 0.4f, 0.6f);
struct fill fill_clskel		= FILL_INITFA(1.0f, 0.6f, 0.6f, 0.6f, FF_SKEL);
struct fill fill_ground		= FILL_INITAB(0.3f, 0.3f, 0.3f, 0.1f, GL_ONE_MINUS_SRC_COLOR);

struct fill fill_borg		= FILL_INIT(0.0f, 0.0f, 0.0f);
struct fill fill_matrix		= FILL_INITF(0.0f, 1.0f, 0.0f, FF_SKEL);
struct fill fill_matrix_reloaded= FILL_INITA(0.0f, 1.0f, 0.0f, 0.3);

__inline void
fill_setrgb(struct fill *fp, float r, float g, float b)
{
	fp->f_r = r;
	fp->f_g = g;
	fp->f_b = b;
}

#if 0
/* Then remove fill arg to draw_*. */
__inline void
fill_apply(struct fill *fp, int flags, int mask)
{
	flags |= fp->f_flags & ~mask;

	if (flags & FF_TEX) {
	} else {
		if (flags & FF_OPAQUE)
			glColor3f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
		else
			glColor4f(fp->f_r, fp->f_g, fp->f_b, fp->f_a);
	}
}
#endif

/*
 * This function is called to gather a number of colors
 * spread out evenly over a "color wheel", providing
 * the best contrast between any two.
 *
 * For optimal colors, we would like to choose colors
 * in HSV mode where hue ranges from 0-360, saturation
 * ranges from 30%-100%, and value ranges from 50%-100%.
 */
void
col_get(int old, size_t n, size_t total, struct fill *fillp)
{
	float hinc, sinc, vinc;
	struct fill *scfp;

	/* Divide color wheel up evenly */
	hinc = 360.0 / (float)total;

	/* These could be random ... */
	sinc = (SAT_MAX - SAT_MIN) / (float)total;
	vinc = (VAL_MAX - VAL_MIN) / (float)total;

	fillp->f_h = hinc * n + HUE_MIN;
	fillp->f_s = sinc * n + SAT_MIN;
	fillp->f_v = vinc * n + VAL_MIN;
	if (!old) {
		fillp->f_a = 1.0;

		scfp = &statusclass[SC_USED].nc_fill;	/* XXX */
		fillp->f_texid[WINID_LEFT]    = scfp->f_texid[WINID_LEFT];
		fillp->f_texid[WINID_RIGHT]   = scfp->f_texid[WINID_RIGHT];
		fillp->f_texid_a[WINID_LEFT]  = scfp->f_texid_a[WINID_LEFT];
		fillp->f_texid_a[WINID_RIGHT] = scfp->f_texid_a[WINID_RIGHT];
	}

	col_hsv_to_rgb(fillp);
}

/*
 * The following two mathematical algorithms were created from
 * pseudocode found in "Fundamentals of Interactive Computer Graphics".
 */
void
col_rgb_to_hsv(struct fill *c)
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
col_hsv_to_rgb(struct fill *c)
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
		return;
	}

	if (h == 360)
		h = 0;
	h /= 60;

	i = h;
	f = h - i;
	p = v * (1 - s);
	q = v * (1 - (s * f));
	t = v * (1 - (s * (1 - f)));

	switch (i) {
	case 0:	fill_setrgb(c, v, t, p); break;
	case 1: fill_setrgb(c, q, v, p); break;
	case 2: fill_setrgb(c, p, v, t); break;
	case 3: fill_setrgb(c, p, q, v); break;
	case 4:	fill_setrgb(c, t, p, v); break;
	case 5: fill_setrgb(c, v, p, q); break;
	}
}

#define CON_VAL_MAX (0.85 * VAL_MAX)
#define CON_VAL_MIN (0.4 * VAL_MIN)

/* Create a contrasting RGB color. */
void
fill_contrast(struct fill *c)
{
	col_rgb_to_hsv(c);

	/* Rotate 180 degrees */
	c->f_h -= 180;

	if (c->f_h < 0)
		c->f_h += 360;

	if (c->f_v < CON_VAL_MAX)
		c->f_v = VAL_MAX;
	else
		c->f_v = CON_VAL_MIN;

	col_hsv_to_rgb(c);
}

void
fill_setopaque(struct fill *fp)
{
	fp->f_a = 1.0;
}

void
fill_setxparent(struct fill *fp)
{
	fp->f_a = 0.0;
}

void
fill_alphainc(struct fill *fp)
{
	fp->f_a += 0.1;
	if (fp->f_a > 1.0)
		fp->f_a = 1.0;
}

void
fill_alphadec(struct fill *fp)
{
	fp->f_a -= 0.1;
	if (fp->f_a < 0.0)
		fp->f_a = 0.0;
}

void
fill_tex(struct fill *fp)
{
	fp->f_flags |= FF_TEX;
}

void
fill_untex(struct fill *fp)
{
	fp->f_flags &= ~FF_TEX;
}
