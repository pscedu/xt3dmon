/* $Id$ */

#ifndef _XT3DMON_H_
#define _XT3DMON_H_

#include "compat.h"

#include <stdio.h>

/* File chooser flags. */
#define CHF_DIR		(1<<0)

/* Physical machine dimensions -- XXX remove hardcode. */
#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

/* Wired dimensions -- XXX remove hardcode. */
#define WIDIM_WIDTH	11
#define WIDIM_HEIGHT	12
#define WIDIM_DEPTH	16

/* Physical machine dimensions -- XXX remove hardcode. */
#define ROWSPACE	(10.0f)
#define CABSPACE	(5.0f)
#define CAGESPACE	(2.0f)
#define MODSPACE	(0.8f)
#define NODESPACE	(0.2f)

#define NODEWIDTH	(vmodes[VM_PHYS].vm_ndim[GEOM_CUBE].fv_w)
#define NODEHEIGHT	(vmodes[VM_PHYS].vm_ndim[GEOM_CUBE].fv_h)
#define NODEDEPTH	(vmodes[VM_PHYS].vm_ndim[GEOM_CUBE].fv_d)
#define NODESHIFT	(0.6f)

#define MODWIDTH	(NODEWIDTH)
#define MODHEIGHT	(NODEHEIGHT * (NNODES / 2) + NODESPACE * (NNODES / 2 - 1))
#define MODDEPTH	(NODEDEPTH * (NNODES / 2) + \
			    (NODESPACE + NODESHIFT) * (NNODES / 2 - 1))

#define CAGEHEIGHT	(NODEHEIGHT * (NNODES / 2) + \
			    (NODESPACE * (NNODES / 2 - 1)))
#define CABWIDTH	(MODWIDTH * NMODS + MODSPACE * (NMODS - 1))
#define CABHEIGHT 	(CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1))

#define ROWWIDTH	(CABWIDTH * NCABS + CABSPACE * (NCABS - 1))
#define ROWDEPTH	(MODDEPTH)

#define CL_WIDTH	(ROWWIDTH)
#define CL_HEIGHT	(CABHEIGHT)
#define CL_DEPTH	(ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1))

#define XCENTER		(NODESPACE + CL_WIDTH  / 2.0)
#define YCENTER		(NODESPACE + CL_HEIGHT / 2.0)
#define ZCENTER		(NODESPACE + CL_DEPTH  / 2.0)

#define WIV_SWIDTH	(WIDIM_WIDTH  * st.st_winsp.iv_x)
#define WIV_SHEIGHT	(WIDIM_HEIGHT * st.st_winsp.iv_y)
#define WIV_SDEPTH	(WIDIM_DEPTH  * st.st_winsp.iv_z)
#define WIV_CLIPX	(st.st_winsp.iv_x * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPY	(st.st_winsp.iv_y * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPZ	(st.st_winsp.iv_z * vmodes[st.st_vmode].vm_clip)

/* Defined values in data files. */
#define DV_NODATA	(-1)
#define DV_NOAUTH	"???"

struct physcoord {				/* XXX: become just dynamic array */
	int	 pc_r;
	int	 pc_cb;
	int	 pc_cg;
	int	 pc_m;
	int	 pc_n;
};

typedef int (*cmpf_t)(const void *, const void *);

struct buf;

/* arch.c */
void		 arch_init(void);

/* callout.c */
void		 cocb_fps(int);
void		 cocb_datasrc(int);
void		 cocb_tourjob(int);
void		 cocb_autoto(int);

/* dbg.c */
void		 dbg_warn(const char *, ...);
void		 dbg_crash(void);

/* eggs.c */
void		 egg_toggle(int);
void		 egg_borg(void);
void		 egg_matrix(void);

/* main.c */
void		 opt_flip(int);
void		 opt_enable(int);
void		 opt_disable(int);

void		 restart(void);

int		 roundclass(double, double, double, int);

/* mach-parse.y */
void		 parse_machconf(const char *);

/* png.c */
void 		*png_load(char *, unsigned int *, unsigned int *);
void		 png_write(FILE *, unsigned char *, long, long);

/* selnid.c */
void		 selnid_load(void);
void		 selnid_save(void);

/* tex.c */
void		 tex_load(void);

/* text.c */
void		 text_wrap(struct buf *, const char *, size_t, const char *, size_t);

extern int		 exthelp;

/* Program-related variables. */
extern const char	*progname;
extern int		 verbose;

extern long		 fps, fps_cnt;

extern unsigned long	 vmem;
extern long		 rmem;

extern char		 login_auth[BUFSIZ];

extern const struct fvec fv_zero;
extern const struct ivec iv_zero;

extern time_t		 mach_drain;

extern char		 date_fmt[];

#endif	/* _XT3DMON_H_ */
