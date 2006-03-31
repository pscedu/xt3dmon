/* $Id$ */

#ifndef _XT3DMON_H_
#define _XT3DMON_H_

#include "compat.h"

#include <stdio.h>

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define WIDIM_WIDTH	11
#define WIDIM_HEIGHT	12
#define WIDIM_DEPTH	16

#define ROWSPACE	(10.0f)
#define CABSPACE	(5.0f)
#define CAGESPACE	(2.0f)
#define MODSPACE	(0.8f)
#define NODESPACE	(0.2f)

#define NODEWIDTH	(vmodes[st.st_vmode].vm_ndim.fv_w)
#define NODEHEIGHT	(vmodes[st.st_vmode].vm_ndim.fv_h)
#define NODEDEPTH	(vmodes[st.st_vmode].vm_ndim.fv_d)
#define NODESHIFT	(0.6f)

#define MODWIDTH	(NODEWIDTH / 4.0f)
#define MODHEIGHT	(NODEHEIGHT * (NNODES / 2) + NODESPACE * (NNODES / 2 - 1))
#define MODDEPTH	(NODEDEPTH * (NNODES / 2) + NODESPACE * (NNODES / 2 - 1) + NODESHIFT)

#define CAGEHEIGHT	(NODEHEIGHT * (NNODES / 2) + \
			    (NODESPACE * (NNODES / 2 - 1)))
#define CABWIDTH	(MODWIDTH * NMODS + MODSPACE * (NMODS - 1))
#define CABHEIGHT 	(CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1))

#define ROWWIDTH	(CABWIDTH * NCABS + CABSPACE * (NCABS - 1))
#define ROWDEPTH	(NODEDEPTH * (NNODES / 2) + \
			    (NODESPACE * (NNODES / 2 - 1)))

#define XCENTER		(NODESPACE + ROWWIDTH / 2)
#define YCENTER		(NODESPACE + (CAGEHEIGHT * NCAGES + \
			    CAGESPACE * (NCAGES - 1)) / 2.0f)
#define ZCENTER		(NODESPACE + (ROWDEPTH * NROWS + \
			    ROWSPACE * (NROWS - 1)) / 2.0f)

#define WIV_SWIDTH	(WIDIM_WIDTH  * st.st_winsp.iv_x)
#define WIV_SHEIGHT	(WIDIM_HEIGHT * st.st_winsp.iv_y)
#define WIV_SDEPTH	(WIDIM_DEPTH  * st.st_winsp.iv_z)
#define WIV_CLIPX	(st.st_winsp.iv_x * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPY	(st.st_winsp.iv_y * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPZ	(st.st_winsp.iv_z * vmodes[st.st_vmode].vm_clip)

/* Defined values. */
#define DV_NODATA	(-1)

/* Stereo display mode types. */
#define STM_NONE	0
#define STM_ACT		1
#define STM_PASV	2

/* GL window identifiers for passive stereo. */
#define WINID_LEFT	0
#define WINID_RIGHT	1

#define WINID_DEF	0

struct physcoord {				/* XXX: become just dynamic array */
	int	 pc_r;
	int	 pc_cb;
	int	 pc_cg;
	int	 pc_m;
	int	 pc_n;
};

typedef int (*cmpf_t)(const void *, const void *);

/* arch.c */
void		 arch_init(void);

/* callout.c */
void		 cocb_fps(int);
void		 cocb_datasrc(int);
void		 cocb_clearstatus(int);
void		 cocb_tourjob(int);

/* dbg.c */
void		 dbg_warn(const char *, ...);
void		 dbg_crash(void);

/* eggs.c */
void		 egg_borg(void);
void		 egg_matrix(void);

/* hl.c */
void		 hl_change(void);
void		 hl_setall(float);

/* png.c */
void 		*png_load(char *, unsigned int *, unsigned int *);
void		 png_write(FILE *, unsigned char *, long, long);

/* main.c */
void		 opt_flip(int);
void		 opt_enable(int);
void		 opt_disable(int);

void		 restart(void);

int		 roundclass(int, int, int, int);

/* parse-phys.y */
void		 parse_physconf(void);

/* status.c */
void		 status_add(const char *, ...);
void		 status_set(const char *, ...);
void		 status_clear(void);
const char	*status_get(void);

/* tex.c */
void		 tex_load(void);

/* text.c */
void		 text_wrap(char *, size_t, size_t);

extern int		 mode_data_clean;
extern int		 selnode_clean;
extern int		 exthelp;

/* Program-related variables. */
extern const char	*progname;
extern int		 verbose;

extern long		 fps, fps_cnt;

extern unsigned long	 vmem;
extern long		 rmem;

extern char		 login_auth[BUFSIZ];

#endif	/* _XT3DMON_H_ */
