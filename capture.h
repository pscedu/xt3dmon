/* $Id$ */

/* Capture formats. */
#define CM_PNG	0
#define CM_PPM	1

void	 capture_frame(int);
void 	 capture_begin(int);
void	 capture_end(void);
void	 capture_snap(const char *, int);
void	 capture_snapfd(int, int);

extern int	 capture_mode;