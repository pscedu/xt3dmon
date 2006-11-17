/* $Id$ */

#define TWF_LOOK	(1<<0)
#define TWF_POS		(1<<1)
#define TWF_UP		(1<<2)

void	 tween_pop(int);
void	 tween_push(int);
void	 tween_update(void);

extern struct fvec	 tv, tlv, tuv;		/* tween vectors */
