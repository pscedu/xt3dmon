/* $Id$ */

struct node;
struct fvec;
struct fill;

/* Node drawing flags. */
#define NDF_DONTPUSH	(1<<0)
#define NDF_NOOPTS	(1<<1)

/* draw.c */
void	 draw_node(struct node *, int);
void	 draw_panels(int);
void	 draw_shadow_panels(void);

void	 make_ground(void);
void	 make_cluster(void);
void	 make_selnodes(void);

/* widget.c */
void	 draw_box_outline(const struct fvec *, const struct fill *);
void	 draw_box_filled(const struct fvec *, const struct fill *);
void	 draw_box_tex(const struct fvec *, const struct fill *, GLenum);

extern int dl_cluster[2];
extern int dl_ground[2];
extern int dl_selnodes[2];

extern GLUquadric *quadric;
