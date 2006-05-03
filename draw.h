/* $Id$ */

struct node;
struct fvec;
struct fill;

#define DF_FRAME	(1<<0)		/* draw with frame */

/* Node drawing flags. */
#define NDF_ATORIGIN	(1<<0)

/* draw.c */
void	 draw_scene(void);
void	 draw_node(struct node *, int);
void	 draw_panels(int);
void	 draw_shadow_panels(void);

void	 make_ground(void);
void	 make_cluster(void);
void	 make_selnodes(void);

/* widget.c */
void	 draw_cube(const struct fvec *, const struct fill *, int);
void	 draw_sphere(const struct fvec *, const struct fill *, int);

extern int dl_cluster[2];
extern int dl_ground[2];
extern int dl_selnodes[2];

extern GLUquadric *quadric;
