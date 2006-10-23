/* $Id$ */

#include "queue.h"
#include "xmath.h"

#define DX_DEFAULT "full"

/* Set mode values. */
#define DXV_ON		0
#define DXV_OFF		1
#define DXV_SET		2

/* Cycle action methods. */
#define DACM_GROW	0
#define DACM_CYCLE	1

/* Seljob values. */
#define DXSJ_RND	(-1)

/* Selnode values. */
#define DXSN_RND	(-1)
#define DXSN_VIS	(-2)	/* All visible. */

/* SeaStar control types. */
#define DXSST_VC	0
#define DXSST_MODE	1

/* SeaStar modes. */
#define DXSSM_CYCLES	0
#define DXSSM_PKTS	1
#define DXSSM_FLTS	2

struct dx_action {
	TAILQ_ENTRY(dx_action)	 dxa_link;
	int			 dxa_type;
	char			*dxa_str;
	int			 dxa_intg;
	int			 dxa_intg2;
	double			 dxa_dbl;
	double			 dxa_dbl2;
	struct ivec		 dxa_iv;
	struct ivec		 dxa_iv2;
#define dxa_caption	dxa_str
#define dxa_cuban8_dim	dxa_intg
#define dxa_cycle_meth	dxa_intg
#define dxa_dmode	dxa_intg
#define dxa_hl		dxa_intg

#define dxa_move_amt	dxa_dbl
#define dxa_move_dir	dxa_intg
#define dxa_move_secs	dxa_intg2

#define dxa_opt_mode	dxa_intg
#define dxa_opts	dxa_intg2

#define dxa_orbit_dim	dxa_intg2
#define dxa_orbit_dir	dxa_intg
#define dxa_orbit_frac	dxa_dbl
#define dxa_orbit_secs	dxa_dbl2

#define dxa_panel_mode	dxa_intg
#define dxa_panels	dxa_intg2

#define dxa_pipemode	dxa_intg
#define dxa_pstick	dxa_intg

#define dxa_reel	dxa_str
#define dxa_reel_delay	dxa_intg

#define dxa_screw_dim	dxa_intg
#define dxa_seljob	dxa_intg
#define dxa_selnode	dxa_intg

#define dxa_ssctl_type	dxa_intg
#define dxa_ssctl_value	dxa_intg2

#define dxa_stall_secs	dxa_dbl
#define dxa_subselnode	dxa_intg
#define dxa_vmode	dxa_intg

#define dxa_winsp	dxa_iv2
#define dxa_winsp_mode	dxa_iv

#define dxa_wioff	dxa_iv2
#define dxa_wioff_mode	dxa_iv
};

TAILQ_HEAD(dxlist, dx_action);

void  dxa_clear(void);
void  dxa_add(const struct dx_action *);

int   dx_parse(void);
void  dx_start(void);
void  dx_end(void);
char *dx_set(const char *, int);
void  dx_update(void);
void  dx_error(const char *, va_list);
void  dx_verror(const char *, ...);

extern struct dxlist	dxlist;
extern char		dx_fn[];
extern char		dx_dir[];
extern int		dx_active;
