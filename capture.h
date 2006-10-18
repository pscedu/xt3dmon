/* $Id$ */

/* Capture formats. */
#define CM_PNG	0
#define CM_PPM	1

const char	*capture_seqname(int);
void	 	 capture_snap(const char *, int);
void	 	 capture_snapfd(int, int);
void	 	 capture_virtual(void);

extern int	 capture_mode;
