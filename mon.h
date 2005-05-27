/* $Id$ */

#include <sys/queue.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include "slist.h"

/*
#define _PATH_JOBMAP	"/usr/users/torque/nids_list_login%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"
#define _PATH_PHYSMAP	"/opt/tmp-harness/default/ssconfig/sys%d/nodelist"
*/

#define _PATH_JOBMAP	"data/nids_list_login%d"
#define _PATH_PHYSMAP	"data/nodelist%d"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_login%d"
#define _PATH_CHECKMAP	"/usr/users/torque/check_nids_list_login%d"

#define _PATH_TEX	"data/texture%d.png"

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define ST_FREE		0
#define ST_DOWN		1
#define ST_DISABLED	2
#define ST_USED		3
#define ST_IO		4
#define ST_UNACC	5
#define ST_BAD		6
#define ST_CHECK	7
#define ST_INFO		8
#define NST		9

#define OP_TEX		(1<<0)
#define OP_BLEND	(1<<1)
#define OP_WIRES	(1<<2)
#define OP_LINEFOLLOW	(1<<3)
#define OP_LINELEAVE	(1<<4)
#define OP_GROUND	(1<<5)
#define OP_TWEEN	(1<<6)
#define OP_CAPTURE	(1<<7)

#define PANEL_FPS	(1<<0)
#define PANEL_NINFO	(1<<1)

#define PI		(3.14159265358979323)

#define NLOGIDS		(sizeof(logids) / sizeof(logids[0]))

#define SQUARE(x)	((x) * (x))

#define TWEEN_THRES	(0.01f)
#define TWEEN_AMT	(0.05f)
#define TM_STRAIGHT	1
#define TM_RADIUS	2

#define WFRAMEWIDTH	(0.001f)

struct job {
	int		 j_id;
	float		 j_r;
	float		 j_g;
	float		 j_b;
};

struct node {
	int		 n_nid;
	int		 n_logid;
	struct job	*n_job;
	int		 n_state;
	int		 n_savst;
	struct {
		float	 np_x;
		float	 np_y;
		float	 np_z;
	}		 n_pos;
};

struct nstate {
	char		*nst_name;
	float		 nst_r;
	float		 nst_g;
	float		 nst_b;
	int		 nst_texid;
};

struct state {
	float		 st_x;
	float		 st_y;
	float		 st_z;
	float		 st_lx;
	float		 st_ly;
	float		 st_lz;
	int		 st_opts;
	int		 st_panels;
	float		 st_alpha_job;
	float		 st_alpha_oth;
	GLint		 st_alpha_fmt;

	int		 st_tween_mode;
	int		 st_nframes;
};

struct lineseg {
	float			ln_sx;
	float			ln_sy;
	float			ln_sz;
	float			ln_ex;
	float			ln_ey;
	float			ln_ez;
	SLIST_ENTRY(lineseg)	ln_next;
};

SLIST_HEAD(lineseglh, lineseg);

/* draw.c */
void			 draw(void);
void			 draw_node(struct node *, float, float, float);

/* load_png.c */
void			 load_texture(void *, GLint, int);
void 			*load_png(char *);

/* mon.c */
void			 adjcam(void);
void			 calc_flyby(void);

/* panel.c */
void			 draw_fps(void);
void			 draw_node_info(void);

/* parse.c */
void			 parse_jobmap(void);
void			 parse_physmap(void);

/* capture.c */
void			 capture_fb(void);

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

extern int		 active_fps;
extern int		 active_ninfo;
extern int		 active_flyby;

extern struct lineseglh	 seglh;
extern struct nstate	 nstates[];
extern const struct state flybypath[];
