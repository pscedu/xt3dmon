/* $Id$ */

#include <sys/queue.h>
#include <sys/types.h>
#include <stdio.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include "buf.h"
#include "queue.h"

/*
#define _PATH_JOBMAP	"/usr/users/torque/nids_list_login%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"
#define _PATH_PHYSMAP	"/opt/tmp-harness/default/ssconfig/sys%d/nodelist"
*/

#define _PATH_JOBMAP	"data/nids_list_login%d"
#define _PATH_PHYSMAP	"data/nodelist%d"
#define _PATH_FAILMAP	"data/fail"
#define _PATH_TEMPMAP	"data/temps"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"
#define _PATH_FLYBY	"data/flyby.data"
#define _PATH_TEX	"data/texture%d.png"

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define SCALE		(1.0f)

#define ROWSPACE	((10.0f) * SCALE)
#define CABSPACE	((5.0f) * SCALE)
#define CAGESPACE	((1.0f) * SCALE)
#define MODSPACE	((1.0f) * SCALE)

#define MODWIDTH	((1.0f) * SCALE)
#define MODHEIGHT	((2.0f) * SCALE)
#define MODDEPTH	((2.0f) * SCALE)

#define NODESPACE	((0.2f) * SCALE)
#define NODEWIDTH	(MODWIDTH - 2.0f * NODESPACE)
#define NODEHEIGHT	(MODHEIGHT - 4.0f * NODESPACE)
#define NODEDEPTH	(MODHEIGHT - 4.0f * NODESPACE)

#define CAGEHEIGHT	(MODHEIGHT * 2.0f)
#define CABWIDTH	((MODWIDTH + MODSPACE) * NMODS)
#define ROWDEPTH	(MODDEPTH * 2.0f)

#define ROWWIDTH	(CABWIDTH * NCABS + CABSPACE * (NCABS - 1))

#define XCENTER		(ROWWIDTH / 2)
#define YCENTER		((CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1)) / 2)
#define ZCENTER		((ROWDEPTH * NROWS + ROWSPACE * (NROWS - 1)) / 2)

#define JST_FREE	0
#define JST_DOWN	1
#define JST_DISABLED	2
#define JST_USED	3
#define JST_IO		4
#define JST_UNACC	5
#define JST_BAD		6
#define JST_CHECK	7
#define NJST		8

#define PI		(3.14159265358979323)

#define NLOGIDS		(sizeof(logids) / sizeof(logids[0]))

#define NID_MAX		(NROWS * NCABS * NCAGES * NMODS * NNODES)

#define SQUARE(x)	((x) * (x))

#define WFRAMEWIDTH	(0.001f)

struct fill {
	float		 f_r;
	float		 f_g;
	float		 f_b;
	float		 f_a;
	GLint		 f_texid;
};

struct job {
	int		 j_id;
	char		*j_owner;
	char		*j_name;
	int		 j_dur;
	int		 j_cpus;
	struct fill	 j_fill;
};

struct temp {
	int		 t_cel;
	struct fill	 t_fill;
};

struct fail {
	int		 f_fails;
	struct fill	 f_fill;
};

struct vec {
	float		 v_x;
	float		 v_y;
	float		 v_z;
};

struct node {
	int		 n_nid;
	int		 n_logid;
	struct job	*n_job;
	struct temp	*n_temp;
	struct fail	*n_fail;
	int		 n_state;
	struct fill	*n_fillp;
	struct fill	*n_ofillp;
	struct {
		float	 np_x;
		float	 np_y;
		float	 np_z;
	}		 n_pos;
	union {
		CIRCLEQ_HEAD(, node) nu_xhead;
		CIRCLEQ_ENTRY(node) nu_xlink;
	}		 n_uwiredx;
	union {
		CIRCLEQ_HEAD(, node) nu_yhead;
		CIRCLEQ_ENTRY(node) nu_ylink;
	}		 n_uwiredy;
	union {
		CIRCLEQ_HEAD(, node) nu_zhead;
		CIRCLEQ_ENTRY(node) nu_zlink;
	}		 n_uwiredz;
#define n_xhead n_uwiredx.nu_xhead
#define n_yhead n_uwiredy.nu_yhead
#define n_zhead n_uwiredz.nu_zhead

#define n_xlink n_uwiredx.nu_xlink
#define n_ylink n_uwiredy.nu_ylink
#define n_zlink n_uwiredz.nu_zlink
};

struct state {
	float		 st_x;
	float		 st_y;
	float		 st_z;
	float		 st_lx;
	float		 st_ly;
	float		 st_lz;
	int		 st_opts;
	float		 st_alpha_job;
	float		 st_alpha_oth;
	GLint		 st_alpha_fmt;
	int		 st_mode;
	int		 st_vmode;
	int		 st_ro;
};

struct flyby {
	struct state	*fb_state;
	struct {
		int	 fbni_nlid;
		int 	 fbni_nid;
	}		 fb_ninfo;
	int		 fb_panels;
#define fb_nlid	fb_ninfo.fbni_nlid
#define fb_nid	fb_ninfo.fbni_nid
};

#define TM_STRAIGHT	1
#define TM_RADIUS	2

#define RO_TEX		(1<<0)
#define RO_PHYS		(1<<1)
#define RO_RELOAD	(1<<2)
#define RO_COMPILE	(1<<3)
#define RO_TEXTURE	(1<<4)

#define RO_ALL		(RO_TEX | RO_PHYS | RO_RELOAD | RO_COMPILE)

#define OP_TEX		(1<<0)
#define OP_BLEND	(1<<1)
#define OP_WIRES	(1<<2)
#define OP_GROUND	(1<<3)
#define OP_TWEEN	(1<<4)
#define OP_CAPTURE	(1<<5)
#define OP_DISPLAY	(1<<6)

#define SM_JOBS		0
#define SM_FAIL		1
#define SM_TEMP		2

#define VM_PHYSICAL	0
#define VM_LOGICAL	1

struct job_state {
	char		*js_name;
	struct fill	 js_fill;
};

struct uinput {
	struct buf 	  uinp_buf;
	void		(*uinp_callback)(void);
	int		  uinp_panel;
	int		  uinp_opts;
};

#define UINPO_LINGER	(1<<0)

struct pwidget {
	char			 *pw_str;
	struct fill		 *pw_fillp;
	SLIST_ENTRY(pwidget)	  pw_next;
};

struct panel {
	int			  p_id;
	char			 *p_str;
	size_t			  p_strlen;
	int			  p_u;
	int			  p_v;
	int			  p_w;
	int			  p_h;
	int			  p_opts;
	struct fill		  p_fill;
	void			(*p_refresh)(struct panel *);
	TAILQ_ENTRY(panel)	  p_link;
	SLIST_HEAD(, pwidget)	  p_widgets;
	int			  p_nwidgets;
};

#define PANEL_FPS	(1<<0)
#define PANEL_NINFO	(1<<1)
#define PANEL_CMD	(1<<2)
#define PANEL_LEGEND	(1<<3)
#define PANEL_FLYBY	(1<<4)
#define PANEL_GOTO	(1<<5)
#define NPANELS		6

#define POPT_REMOVE	(1<<0)
#define POPT_COMPILE	(1<<1)

TAILQ_HEAD(panels, panel);

/* cam.c */
void			 move_cam(int);
void			 rotate_cam(int, int);

/* draw.c */
void			 draw(void);
void			 draw_node(struct node *, float, float, float);

/* key.c */
void			 keyh_flyby(unsigned char, int, int);
void			 keyh_uinput(unsigned char, int, int);
void			 keyh_panel(unsigned char, int, int);
void			 keyh_mode(unsigned char, int, int);
void			 keyh_vmode(unsigned char, int, int);
void			 keyh_default(unsigned char, int, int);
void			 spkeyh_default(int, int, int);

/* load_png.c */
void			 load_texture(void *, GLint, int);
void 			*load_png(char *);

/* mon.c */
void			 adjcam(void);
void			 calc_flyby(void);
void			 restore_state(int);
void			 rebuild(int);

/* panel.c */
void			 draw_panels(void);
void			 panel_toggle(int);
void			 uinpcb_cmd(void);
void			 uinpcb_goto(void);
void			 uinpcb_goto2(void);

/* parse.c */
void			 parse_jobmap(void);
void			 parse_physmap(void);
void			 parse_failmap(void);
void			 parse_tempmap(void);

/* capture.c */
void			 capture_fb(void);

/* flyby.c */
void 			 begin_flyby(char);
void 			 end_flyby(void);
void			 read_flyby(void);
void			 write_flyby(void);
void			 update_flyby(void);
void			 flip_panels(int);

extern int		 logids[2];
extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[NLOGIDS][NROWS * NCABS * NCAGES * NMODS * NNODES];

extern size_t		 njobs;
extern struct job	**jobs;
extern size_t		 nfails;
extern struct fail	**fails;
extern size_t		 ntemps;
extern struct temp	**temps;

extern int		 total_failures;	/* total shared among all nodes */

extern GLint		 cluster_dl;
extern struct state	 st;
extern struct flyby	 fb;
extern long		 fps;
extern struct panels	 panels;
extern struct node	*selnode;

extern struct uinput	 uinp;
extern int		 goto_logid;
extern int		 spkey;

extern float		 tx, tlx, ox, olx;
extern float		 ty, tly, oy, oly;
extern float		 tz, tlz, oz, olz;

extern int		 active_flyby;
extern int		 build_flyby;

extern int		 win_width;
extern int		 win_height;

extern struct job_state	 jstates[];
extern struct fail_state fstates[];
extern struct temp_state tstates[];
