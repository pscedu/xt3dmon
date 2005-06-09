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
#define JST_INFO	8
#define NJST		9

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

#define PANEL_FPS	(1<<0)
#define PANEL_NINFO	(1<<1)
#define PANEL_CMD	(1<<2)
#define PANEL_LEGEND	(1<<3)
#define NPANELS		4

#define RO_TEX		(1<<0)
#define RO_PHYS		(1<<1)
#define RO_RELOAD	(1<<2)
#define RO_COMPILE	(1<<3)

#define RO_ALL		(RO_TEX | RO_PHYS | RO_RELOAD | RO_COMPILE)

#define PI		(3.14159265358979323)

#define NLOGIDS		(sizeof(logids) / sizeof(logids[0]))

#define SQUARE(x)	((x) * (x))

#define TWEEN_THRES	(0.01f)
#define TWEEN_AMT	(0.05f)
#define TM_STRAIGHT	1
#define TM_RADIUS	2

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
	const char	*j_owner;
	const char	*j_name;
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

struct node {
	int		 n_nid;
	int		 n_logid;
	struct job	*n_job;
	struct temp	*n_temp;
	struct fail	*n_fail;
	int		 n_state;
	int		 n_savst;
	struct fill	*n_fillp;
	struct {
		float	 np_x;
		float	 np_y;
		float	 np_z;
	}		 n_pos;
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
	struct node	*st_selnode;

	int		 st_panels;
	int		 st_tween_mode;
	int		 st_nframes;
	int		 st_ro;
};

struct job_state {
	char		*js_name;
	struct fill	 js_fill;
};

struct panel {
	int		 p_id;
	char		*p_str;
	size_t		 p_strlen;
	int		 p_u;
	int		 p_v;
	int		 p_su;
	int		 p_sv;
	int		 p_adju;
	int		 p_adjv;
	int		 p_w;
	int		 p_h;
	struct fill	 p_fill;
	void		(*p_refresh)(struct panel *);
	TAILQ_ENTRY(panel) p_link;
};

TAILQ_HEAD(panels, panel);

/* draw.c */
void			 draw(void);
void			 draw_node(struct node *, float, float, float);

/* key.c */
void			 keyh_flyby(unsigned char, int, int);
void			 keyh_cmd(unsigned char, int, int);
void			 keyh_panel(unsigned char, int, int);
void			 keyh_mode(unsigned char, int, int);
void			 keyh_default(unsigned char, int, int);
void			 spkeyh_default(int, int, int);

/* load_png.c */
void			 load_texture(void *, GLint, int);
void 			*load_png(char *);

/* mon.c */
void			 adjcam(void);
void			 calc_flyby(void);
void			 restore_state(void);
void			 rebuild(int);

/* panel.c */
void			 draw_panels(void);
void			 adjpanels(void);
void			 panel_toggle(int);

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

extern int		 logids[2];
extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[NLOGIDS][NROWS * NCABS * NCAGES * NMODS * NNODES];
extern size_t		 njobs;
extern struct job	**jobs;
extern GLint		 cluster_dl;
extern struct state	 st;
extern long		 fps;

extern float		 tx, tlx;
extern float		 ty, tly;
extern float		 tz, tlz;

extern int		 active_flyby;
extern int		 build_flyby;
extern FILE		*flyby_fp;

extern int		 win_width;
extern int		 win_height;

extern struct job_state	 jstates[];
extern struct fail_state fstates[];
extern struct temp_state tstates[];
extern const struct state flybypath[];

extern struct panels	 panels;
extern struct buf 	 cmdbuf;
extern int		 total_failures;	/* total shared among all nodes */
extern struct fail_state **fail_states;
extern size_t		 maxfails;		/* largest # of failures */
