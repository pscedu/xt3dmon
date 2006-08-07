/* $Id$ */

struct fvec;

void	 cam_move(int, float);
void	 cam_revolve(struct fvec *, int, double, double, int);
void	 cam_revolvefocus(double, double, int);
void	 cam_rotate(const struct fvec *, const struct fvec *, int, int);
void	 cam_roll(double);
void	 cam_look(void);

void	 cam_bird(void);

#define REVT_LKAVG	0	/* Average look across center. */
#define REVT_LKCEN	1	/* Always look at center. */
