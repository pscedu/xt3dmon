/* $Id$ */

struct node;
struct fvec;
struct fill;

/*
 * Determines the texture coordinate by finding the ratio
 * between the pixel value given and the max of the three
 * possible dimensions.  This gives a section of a texture
 * without stretching it.
 */
#define TEXCOORD(x, max) (1.0 / (max / x))

#define DF_FRAME	(1<<0)		/* draw with frame */

/* Node drawing flags. */
#define NDF_NOTWEEN	(1<<0)		/* don't perform node tweening */

/* draw.c */
void	 draw_scene(void);
void	 draw_node(struct node *, int);
void	 draw_panels(int);
void	 draw_shadow_panels(void);
void	 draw_compass(int, int, int, int);

void	 make_ground(void);
void	 make_cluster(void);
void	 make_selnodes(void);

/* widget.c */
void	 draw_cube(const struct fvec *, const struct fill *, int);
void	 draw_sphere(const struct fvec *, const struct fill *, int);

extern unsigned int dl_cluster[2];
extern unsigned int dl_ground[2];
extern unsigned int dl_selnodes[2];

extern GLUquadric *quadric;
