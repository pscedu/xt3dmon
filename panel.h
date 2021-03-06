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

#include "fill.h"
#include "queue.h"
#include "select.h"

#define PANEL_FPS	(1 <<  0)
#define PANEL_NINFO	(1 <<  1)
#define PANEL_CMD	(1 <<  2)
#define PANEL_LEGEND	(1 <<  3)
#define PANEL_FLYBY	(1 <<  4)
#define PANEL_GOTONODE	(1 <<  5)
#define PANEL_POS	(1 <<  6)
#define PANEL_SS	(1 <<  7)
#define PANEL_STATUS	(1 <<  8)
#define PANEL_EGGS	(1 <<  9)
#define PANEL_DATE	(1 << 10)
#define PANEL_OPTS	(1 << 11)
#define PANEL_GOTOJOB	(1 << 12)
#define PANEL_PANELS	(1 << 13)
#define PANEL_LOGIN	(1 << 14)
#define PANEL_HELP	(1 << 15)
#define PANEL_VMODE	(1 << 16)
#define PANEL_DMODE	(1 << 17)
#define PANEL_REEL	(1 << 18)
#define PANEL_PIPE	(1 << 19)
#define PANEL_WIADJ	(1 << 20)
#define PANEL_RT	(1 << 21)
#define PANEL_FBCHO	(1 << 22)
#define PANEL_FBNEW	(1 << 23)
#define PANEL_CMP	(1 << 24)
#define PANEL_KEYH	(1 << 25)
#define PANEL_DXCHO	(1 << 26)
#define PANEL_DSCHO	(1 << 27)
#define PANEL_VNMODE	(1 << 28)
#define PANEL_PIPEDIM	(1 << 29)
#define NPANELS		30

struct pwidget;
TAILQ_HEAD(pwidget_group_list, pwidget);

struct pwidget_group {
	struct pwidget_group_list  pwg_widgets;
	SLIST_ENTRY(pwidget_group) pwg_link;
	struct pwidget		  *pwg_checkedwidget;
};

struct pwidget {
	const char		 *pw_str;
	int			  pw_flags;
	int			  pw_sprio;	/* Sort priority. */
	struct fill		 *pw_fillp;
	SLIST_ENTRY(pwidget)	  pw_next;
	struct pwidget_group	 *pw_group;
	TAILQ_ENTRY(pwidget)	  pw_group_link;
	struct glname		 *pw_gn;
	gscb_t			  pw_cb;
	int			  pw_arg_int;
	int			  pw_arg_int2;
	void			 *pw_arg_ptr;
	void			 *pw_arg_ptr2;
	int			  pw_u;
	int			  pw_v;
	int			  pw_w;
	int			  pw_h;
};

#define PWF_HIDE		(1 << 0)

struct pinfo;

struct panel {
	int			  p_id;
	struct pinfo		 *p_info;
	int			  p_dl[2];	/* GL display list. */
	char			 *p_str;
	size_t			  p_strlen;
	int			  p_u;
	int			  p_v;
	int			  p_w;
	int			  p_h;
	int			  p_opts;
	void			(*p_refresh)(struct panel *);
	struct fill		  p_fill;
	TAILQ_ENTRY(panel)	  p_link;
	SLIST_HEAD(, pwidget)	  p_widgets;
	struct pwidget		 *p_lastwidget;
	SLIST_HEAD(, pwidget)	  p_freewidgets;
	SLIST_HEAD(, pwidget_group) p_widgetgroups;
	struct pwidget_group	 *p_curwidgetgroup;
	int			  p_nwidgets;
	int			  p_nwcol;	/* Number of widget columns. */
	int			  p_totalwcw;	/* Total width of all columns combined. */
	size_t			 *p_wcw;	/* Widths of individual columns. */
	void			(*p_extdrawf)(struct panel *);
};

#define POPT_REMOVE		(1 << 0)	/* being removed */
#define POPT_DIRTY		(1 << 1)	/* panel needs redrawn, but same contents */
#define POPT_REFRESH		(1 << 2)	/* panel needs contents rebuilt */
#define POPT_MOBILE		(1 << 3)	/* being dragged */
#define POPT_COMPILE		(1 << 4)	/* stereo: panel was compiled */
#define POPT_USR1		(1 << 5)	/* panel-specific flag */
#define POPT_CANSYNC		(1 << 6)	/* stereo: just sync draw */

#define POPT_LOGIN_ATPASS (POPT_USR1)

TAILQ_HEAD(panels, panel);

struct pinfo {
	const char		 *pi_abbr;
	const char		 *pi_name;
	void			(*pi_refresh)(struct panel *);
	int			  pi_stick;
	int			  pi_flags;
	int			  pi_uinpopts;
	void			(*pi_uinpcb)(void);
};

#define PIF_UINP		(1 << 0)
#define PIF_XPARENT		(1 << 1)	/* panel is transparent */
#define PIF_HIDE		(1 << 2)
#define PIF_FBIGN		(1 << 3)	/* ignore in flybys */

#define PSTICK_TL		1
#define PSTICK_TR		2
#define PSTICK_BL		3
#define PSTICK_BR		4

void		 pwidget_grouplist_free(struct panel *);

void		 panel_toggle(int);
void		 panel_tremove(struct panel *);
void		 panel_show(int);
void		 panel_hide(int);
struct panel	*panel_for_id(int);
void		 panel_demobilize(struct panel *);
void		 panel_draw_compass(struct panel *);
void		 panel_rebuild(int);
void		 panel_redraw(int);
void		 panel_calcwlens(struct panel *);

void		 panels_set(int);
void		 panels_flip(int);
void		 panels_show(int);
void		 panels_hide(int);

void panel_refresh_cmd(struct panel *);
void panel_refresh_cmp(struct panel *);
void panel_refresh_date(struct panel *);
void panel_refresh_dmode(struct panel *);
void panel_refresh_dscho(struct panel *);
void panel_refresh_dxcho(struct panel *);
void panel_refresh_eggs(struct panel *);
void panel_refresh_fbcho(struct panel *);
void panel_refresh_fbnew(struct panel *);
void panel_refresh_flyby(struct panel *);
void panel_refresh_fps(struct panel *);
void panel_refresh_gotojob(struct panel *);
void panel_refresh_gotonode(struct panel *);
void panel_refresh_help(struct panel *);
void panel_refresh_keyh(struct panel *);
void panel_refresh_legend(struct panel *);
void panel_refresh_login(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_opts(struct panel *);
void panel_refresh_panels(struct panel *);
void panel_refresh_pipe(struct panel *);
void panel_refresh_pos(struct panel *);
void panel_refresh_reel(struct panel *);
void panel_refresh_rt(struct panel *);
void panel_refresh_ss(struct panel *);
void panel_refresh_status(struct panel *);
void panel_refresh_vmode(struct panel *);
void panel_refresh_wiadj(struct panel *);
void panel_refresh_vnmode(struct panel *);
void panel_refresh_pipedim(struct panel *);

extern struct panels	 panels;
extern struct pinfo	 pinfo[];
extern struct panel	*panel_mobile;

extern int		 dmode_data_clean;
extern int		 selnode_clean;
extern int		 hlnc_clean;
