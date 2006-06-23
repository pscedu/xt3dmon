/* $Id$ */

#include "queue.h"
#include "xmath.h"

struct dx_action {
	TAILQ_ENTRY(dx_action)	 dxa_link;
	int			 dxa_action;
	char			*dxa_captiontext;
	int			 dxa_intg;
	int			 dxa_intg2;
	struct ivec		 dxa_iv;
	struct ivec		 dxa_iv2;
#define dxa_dmode	dxa_intg
#define dxa_vmode	dxa_intg
#define dxa_hl		dxa_intg

#define dxa_move_dir	dxa_intg
#define dxa_move_amt	dxa_intg2

#define dxa_opt_mode	dxa_intg
#define dxa_opts	dxa_intg2

#define dxa_panel_mode	dxa_intg
#define dxa_panels	dxa_intg2

#define dxa_orbit_dir	dxa_intg
#define dxa_orbit_dim	dxa_intg2

#define dxa_seljob	dxa_intg
#define dxa_selnode	dxa_intg

#define dxa_winsp_mode	dxa_iv
#define dxa_winsp	dxa_iv2

#define dxa_wioff_mode	dxa_iv
#define dxa_wioff	dxa_iv2
};

void dxa_clear(void);
void dxa_add(struct dx_action *);

void dx_update(void);
void dx_error(const char *, va_list);
void dx_verror(const char *, ...);
