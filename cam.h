/* $Id$ */

struct fvec;

void	 cam_move(int, float);
void	 cam_revolve(struct fvec *, float, float);
void	 cam_rotate(const struct fvec *, const struct fvec *, int, int);
void	 cam_roll(double);
void	 cam_look(void);
void	 cam_getspecvec(struct fvec *, int, int);
