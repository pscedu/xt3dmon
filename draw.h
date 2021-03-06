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

struct node;
struct fvec;
struct fill;

/*
 * Determines the texture coordinate by finding the ratio
 * between the pixel value given and the max of the three
 * possible dimensions.  This gives a section of a texture
 * without stretching it.
 */
#define TEXCOORD(x, max) (1.0 / ((max) / (x)))

#define DF_FRAME	(1 << 0)	/* draw with frame */

/* Node drawing flags. */
#define NDF_NOTWEEN	(1 << 0)	/* don't perform node tweening */
#define NDF_IGNSN	(1 << 1)	/* ignore selnode mods */

__BEGIN_DECLS

/* draw.c */
void	 draw_scene(void);
void	 draw_node(struct node *, int);
void	 draw_panels(int);
void	 draw_shadow_panels(void);
void	 draw_compass(int, int, int, int);
void	 draw_info(const char *);

void	 make_ground(void);
void	 make_cluster(void);
void	 make_selnodes(void);

void	 draw_text(const char *, struct fvec *, struct fill *);

/* widget.c */
void	 draw_cube(const struct fvec *, const struct fill *, int);
void	 draw_sphere(const struct fvec *, const struct fill *, int);
void	 draw_square(const struct fvec *, const struct fill *);

__END_DECLS

extern unsigned int dl_cluster[2];
extern unsigned int dl_ground[2];
extern unsigned int dl_selnodes[2];

extern GLUquadric *quadric;
