/* $Id$ */

#include "fill.h"
#include "queue.h"

#define PANEL_FPS	(1<<0)
#define PANEL_NINFO	(1<<1)
#define PANEL_CMD	(1<<2)
#define PANEL_LEGEND	(1<<3)
#define PANEL_FLYBY	(1<<4)
#define PANEL_GOTONODE	(1<<5)
#define PANEL_POS	(1<<6)
#define PANEL_SS	(1<<7)
#define PANEL_STATUS	(1<<8)
#define PANEL_MEM	(1<<9)
#define PANEL_EGGS	(1<<10)
#define PANEL_DATE	(1<<11)
#define PANEL_OPTS	(1<<12)
#define PANEL_GOTOJOB	(1<<13)
#define PANEL_PANELS	(1<<14)
#define PANEL_LOGIN	(1<<15)
#define PANEL_HELP	(1<<16)
#define PANEL_VMODE	(1<<17)
#define PANEL_DMODE	(1<<18)
#define PANEL_REEL	(1<<19)
#define NPANELS		20

struct pwidget {
	const char		 *pw_str;
	struct fill		 *pw_fillp;
	SLIST_ENTRY(pwidget)	  pw_next;
	void			(*pw_cb)(int);
	int			  pw_cbarg;
	int			  pw_u;
	int			  pw_v;
	int			  pw_w;
	int			  pw_h;
};

struct panel {
	int			  p_id;
	int			  p_dl[2];
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
	struct pwidget		**p_nextwidget;
	int			  p_nwidgets;
	size_t			  p_maxwlen;
};

#define POPT_REMOVE	(1<<0)			/* being removed */
#define POPT_DIRTY	(1<<1)			/* panel needs redrawn */
#define POPT_USRDIRTY	(1<<2)			/* user-flag for the same */
#define POPT_MOBILE	(1<<3)			/* being dragged */
#define POPT_COMPILE	(1<<4)			/* stereo syncing */
#define POPT_USR1	(1<<5)			/* panel-specific flag */

#define POPT_LOGIN_ATPASS (POPT_USR1)

TAILQ_HEAD(panels, panel);

struct pinfo {
	char		 *pi_name;
	void		(*pi_refresh)(struct panel *);
	int		  pi_stick;
	int		  pi_opts;
	int		  pi_uinpopts;
	void		(*pi_uinpcb)(void);
};

#define PF_UINP		(1<<0)
#define PF_XPARENT	(1<<1)			/* panel is transparent */
#define PF_HIDE		(1<<2)
#define PF_FBIGN	(1<<3)			/* ignore in flybys */

#define PSTICK_TL	1
#define PSTICK_TR	2
#define PSTICK_BL	3
#define PSTICK_BR	4

void		 panel_toggle(int);
void		 panel_tremove(struct panel *);
void		 panel_show(int);
void		 panel_hide(int);
struct panel	*panel_for_id(int);
void		 panel_demobilize(struct panel *);


void panel_refresh_fps(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_cmd(struct panel *);
void panel_refresh_legend(struct panel *);
void panel_refresh_flyby(struct panel *);
void panel_refresh_gotonode(struct panel *);
void panel_refresh_pos(struct panel *);
void panel_refresh_ss(struct panel *);
void panel_refresh_status(struct panel *);
void panel_refresh_mem(struct panel *);
void panel_refresh_eggs(struct panel *);
void panel_refresh_date(struct panel *);
void panel_refresh_opts(struct panel *);
void panel_refresh_gotojob(struct panel *);
void panel_refresh_panels(struct panel *);
void panel_refresh_login(struct panel *);
void panel_refresh_help(struct panel *);
void panel_refresh_vmode(struct panel *);
void panel_refresh_dmode(struct panel *);
void panel_refresh_reel(struct panel *);

extern struct panels	 panels;
extern struct pinfo	 pinfo[];
extern struct panel	*panel_mobile;
