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
	float		f_rgba[4];
	int	 	f_flags;
	int		f_blfunc;
	unsigned int	f_texid[2];
	unsigned int	f_texid_a[2];		/* alpha-loaded texid */
#define f_r f_rgba[0]
#define f_g f_rgba[1]
#define f_b f_rgba[2]
#define f_a f_rgba[3]

#define f_h f_rgba[0]
#define f_s f_rgba[1]
#define f_v f_rgba[2]
};

#define FF_SKEL		(1<<0)			/* Fill is outline, not solid. */
#define FF_TEX		(1<<1)			/* Textured. */
#define FF_OPAQUE	(1<<2)			/* Ignore alpha channel. */

#define FILL_INIT(r, g, b)				\
	FILL_INITFAB((r), (g), (b), 1.0, 0, 0)

#define FILL_INITA(r, g, b, a)				\
	FILL_INITFAB((r), (g), (b), (a), 0, 0)

#define FILL_INITAB(r, g, b, a, blf)			\
	FILL_INITFAB((r), (g), (b), (a), 0, (blf))

#define FILL_INITF(r, g, b, flags)			\
	FILL_INITFAB((r), (g), (b), 1.0, (flags), 0)

#define FILL_INITFA(r, g, b, a, flags)			\
	FILL_INITFAB((r), (g), (b), (a), (flags), 0)

#define FILL_INITFAB(r, g, b, a, flags, blf)		\
	{ { (r), (g), (b), (a) }, (flags), (blf), { 0, 0 }, { 0, 0 } }

void 	 fill_contrast(struct fill *);
void	 fill_setopaque(struct fill *);
void	 fill_setxparent(struct fill *);
void	 fill_alphainc(struct fill *);
void	 fill_alphadec(struct fill *);
void	 fill_tex(struct fill *);
void	 fill_untex(struct fill *);

void	 col_hsv_to_rgb(struct fill *);
void	 col_get(int, size_t, size_t, struct fill *);
void	 col_get_intv(int *, struct fill *);

extern struct fill	 fill_black;
extern struct fill	 fill_grey;
extern struct fill	 fill_light_blue;
extern struct fill	 fill_white;
extern struct fill	 fill_xparent;
extern struct fill	 fill_yellow;
extern struct fill	 fill_pink;
extern struct fill	 fill_green;

extern struct fill	 fill_bg;
extern struct fill	 fill_font;
extern struct fill	 fill_nodata;
extern struct fill	 fill_same;
extern struct fill	 fill_selnode;
extern struct fill	 fill_clskel;
extern struct fill	 fill_ground;

extern struct fill	 fill_psc;
extern struct fill	 fill_borg;
extern struct fill	 fill_matrix;
extern struct fill	 fill_matrix_reloaded;

#endif /* _FILL_H_ */
