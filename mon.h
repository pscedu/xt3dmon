/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#ifndef _XT3DMON_H_
#define _XT3DMON_H_

#include "compat.h"

#include <stdio.h>

#include "phys.h"

/* File chooser flags. */
#define CHF_DIR		(1 << 0)

/* Physical machine dimensions -- XXX remove hardcode. */
#define NPARTS		physdim_top->pd_mag
#define NRACKS		physdim_top->pd_contains->pd_mag
#define NIRUS		physdim_top->pd_contains->pd_contains->pd_mag
#define NBLADES		physdim_top->pd_contains->pd_contains->pd_contains->pd_mag
#define NNODES		(physdim_top->pd_contains->pd_contains->pd_contains->pd_contains->pd_mag)

#define ROWSPACE	physdim_top->pd_space
#define ROWDEPTH	physdim_top->pd_size.fv_d

#define CABSPACE	physdim_top->pd_contains->pd_space
#define CABWIDTH	physdim_top->pd_contains->pd_size.fv_w

#define CAGESPACE	physdim_top->pd_contains->pd_contains->pd_space
#define CAGEHEIGHT	physdim_top->pd_contains->pd_contains->pd_size.fv_h

#define MODWIDTH	physdim_top->pd_contains->pd_contains->pd_contains->pd_size.fv_w
#define MODDEPTH	physdim_top->pd_contains->pd_contains->pd_contains->pd_size.fv_d
#define MODSPACE	physdim_top->pd_contains->pd_contains->pd_contains->pd_space

#define NODESPACE	physdim_top->pd_contains->pd_contains->pd_contains->pd_contains->pd_space
#define NODESHIFT	physdim_top->pd_contains->pd_contains->pd_contains->pd_contains->pd_offset.fv_d
#define NODEWIDTH	physdim_top->pd_contains->pd_contains->pd_contains->pd_contains->pd_size.fv_w
#define NODEHEIGHT	physdim_top->pd_contains->pd_contains->pd_contains->pd_contains->pd_size.fv_h
#define NODEDEPTH	physdim_top->pd_contains->pd_contains->pd_contains->pd_contains->pd_size.fv_d

#define WIV_SWIDTH	(widim.iv_w * st.st_winsp.iv_x)
#define WIV_SHEIGHT	(widim.iv_h * st.st_winsp.iv_y)
#define WIV_SDEPTH	(widim.iv_d * st.st_winsp.iv_z)
#define WIV_CLIPX	(st.st_winsp.iv_x * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPY	(st.st_winsp.iv_y * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPZ	(st.st_winsp.iv_z * vmodes[st.st_vmode].vm_clip)

/* Defined values in data files. */
#define DV_NODATA	(-1)
#define DV_NOAUTH	"???"

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

/* main.c */
void		 opt_set(int);
void		 opt_flip(int);
void		 opt_enable(int);
void		 opt_disable(int);

void		 restart(void);

int		 roundclass(double, double, double, int);

/* mach-parse.y */
void		 parse_machconf(const char *);

/* png.c */
void		*png_load(char *, unsigned int *, unsigned int *);
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

extern char		 login_auth[BUFSIZ];

extern const struct fvec fv_zero;
extern const struct ivec iv_zero;

extern time_t		 mach_drain;

extern char		 date_fmt[];

#endif	/* _XT3DMON_H_ */
