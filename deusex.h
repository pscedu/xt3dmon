/* $Id$ */

#include "queue.h"
#include "xmath.h"

#define DX_DEFAULT "full"

/* Set mode values. */
#define DXV_ON		0
#define DXV_OFF		1
#define DXV_SET		2

/* Seljob values. */
#define DXSJ_RND	(-1)

/* Selnode values. */
#define DXSN_RND	(-1)
#define DXSN_VIS	(-2)	/* All visible. */

struct dx_action {
	TAILQ_ENTRY(dx_action)	 dxa_link;
	int			 dxa_type;
	char			*dxa_caption;
	int			 dxa_intg;
	int			 dxa_intg2;
	double			 dxa_dbl;
	struct ivec		 dxa_iv;
	struct ivec		 dxa_iv2;
#define dxa_dmode	dxa_intg
#define dxa_vmode	dxa_intg
#define dxa_hl		dxa_intg

#define dxa_move_dir	dxa_intg
#define dxa_move_amt	dxa_dbl

#define dxa_opt_mode	dxa_intg
#define dxa_opts	dxa_intg2

#define dxa_panel_mode	dxa_intg
#define dxa_panels	dxa_intg2

#define dxa_pstick	dxa_intg

#define dxa_pipemode	dxa_intg

#define dxa_orbit_dir	dxa_intg
#define dxa_orbit_dim	dxa_intg2

#define dxa_seljob	dxa_intg
#define dxa_selnode	dxa_intg
#define dxa_subselnode	dxa_intg

#define dxa_winsp_mode	dxa_iv
#define dxa_winsp	dxa_iv2

#define dxa_wioff_mode	dxa_iv
#define dxa_wioff	dxa_iv2
};

TAILQ_HEAD(dxlist, dx_action);

void  dxa_clear(void);
void  dxa_add(struct dx_action *);

void  dx_parse(void);
void  dx_start(void);
void  dx_end(void);
char *dx_set(const char *, int);
void  dx_update(void);
void  dx_error(const char *, va_list);
void  dx_verror(const char *, ...);

extern struct dxlist	dxlist;
extern int		dx_built;
extern char		dx_fn[];
extern char		dx_dir[];
int			dx_active;
