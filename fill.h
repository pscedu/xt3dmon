/* $Id$ */

#ifndef _FILL_H_
#define _FILL_H_

#include <sys/types.h>

/* HSV constants. */
#define HUE_MIN		0
#define HUE_MAX		360
#define SAT_MIN		(0.3)
#define SAT_MAX		(1.0)
#define VAL_MIN		(0.5)
#define VAL_MAX		(1.0)

struct fill {
	float	 	f_r;
	float	 	f_g;
	float	 	f_b;
	float	 	f_a;
	int	 	f_flags;
	unsigned int	f_texid[2];
	unsigned int	f_texid_a[2];		/* alpha-loaded texid */
#define f_h f_r
#define f_s f_g
#define f_v f_b
};

#define FF_SKEL		(1<<0)

#define FILL_INIT(r, g, b)				\
	FILL_INITFA((r), (g), (b), 1.0, 0)

#define FILL_INITA(r, g, b, a)				\
	FILL_INITFA((r), (g), (b), (a), 0)

#define FILL_INITF(r, g, b, flags)			\
	FILL_INITFA((r), (g), (b), 1.0, (flags))

#define FILL_INITFA(r, g, b, a, flags)			\
	{ r, g, b, a, flags, { 0, 0 }, { 0, 0 } }

void 	 fill_contrast(struct fill *);

void	 col_hsv_to_rgb(struct fill *);
void	 col_get(int, size_t, size_t, struct fill *);

extern struct fill	 fill_black;
extern struct fill	 fill_grey;
extern struct fill	 fill_light_blue;
extern struct fill	 fill_white;
extern struct fill	 fill_xparent;
extern struct fill	 fill_yellow;

extern struct fill	 fill_bg;
extern struct fill	 fill_font;
extern struct fill	 fill_nodata;
extern struct fill	 fill_same;
extern struct fill	 fill_selnode;

extern struct fill	 fill_borg;
extern struct fill	 fill_matrix;
extern struct fill	 fill_matrix_reloaded;

#endif /* _FILL_H_ */
