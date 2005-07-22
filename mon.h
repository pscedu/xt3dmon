/* $Id$ */

#include <sys/queue.h>

#include <stdio.h>

#include <mysql.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include "buf.h"
#include "queue.h"

#define _PATH_JOBMAP	"data/nids_list_phantom"
#define _PATH_PHYSMAP	"data/rtrtrace"
#define _PATH_FAILMAP	"data/fail"
#define _PATH_TEMPMAP	"data/temps"
#define _PATH_BADMAP	"/usr/users/torque/bad_nids_list_phantom"
#define _PATH_CHECKMAP	"/usr/users/torque/to_check_nids_list_phantom"
#define _PATH_FLYBY	"data/flyby.data"
#define _PATH_TEX	"data/texture%d.png"
#define _PATH_FONT	"data/font.png"

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define ROWSPACE	(10.0f)
#define CABSPACE	(5.0f)
#define CAGESPACE	(2.0f)
#define MODSPACE	(0.8f)
#define NODESPACE	(0.2f)

#define NODEWIDTH	(vmodes[st.st_vmode].vm_ndim.v_w)
#define NODEHEIGHT	(vmodes[st.st_vmode].vm_ndim.v_h)
#define NODEDEPTH	(vmodes[st.st_vmode].vm_ndim.v_d)
#define NODESHIFT	(0.6f)

#define MODWIDTH	(NODEWIDTH / 4.0f)
#define MODHEIGHT	(NODEHEIGHT * (NNODES / 2) + NODESPACE * (NNODES / 2 - 1))
#define MODDEPTH	(NODEDEPTH * (NNODES / 2) + NODESPACE * (NNODES / 2 - 1) + NODESHIFT)

#define CAGEHEIGHT	(NODEHEIGHT * (NNODES / 2) + \
			    (NODESPACE * (NNODES / 2 - 1)))
#define CABWIDTH	(MODWIDTH * NMODS + MODSPACE * (NMODS - 1))

#define ROWWIDTH	(CABWIDTH * NCABS + CABSPACE * (NCABS - 1))
#define ROWDEPTH	(NODEDEPTH * (NNODES / 2) + \
			    (NODESPACE * (NNODES / 2 - 1)))

#define XCENTER		(NODESPACE + ROWWIDTH / 2)
#define YCENTER		(NODESPACE + (CAGEHEIGHT * NCAGES + \
			    CAGESPACE * (NCAGES - 1)) / 2.0f)
#define ZCENTER		(NODESPACE + (ROWDEPTH * NROWS + \
			    ROWSPACE * (NROWS - 1)) / 2.0f)

#define WI_WIDTH	(wired_width * st.st_winspx)
#define WI_HEIGHT	(wired_height * st.st_winspy)
#define WI_DEPTH	(wired_depth * st.st_winspz)
#define WI_CLIPX	(st.st_winspx * vmodes[st.st_vmode].vm_clip)
#define WI_CLIPY	(st.st_winspy * vmodes[st.st_vmode].vm_clip)
#define WI_CLIPZ	(st.st_winspz * vmodes[st.st_vmode].vm_clip)

#define WFRAMEWIDTH	(0.001f)

#define JST_FREE	0
#define JST_DOWN	1
#define JST_DISABLED	2
#define JST_USED	3
#define JST_IO		4
#define JST_UNACC	5
#define JST_BAD		6
#define JST_CHECK	7
#define NJST		8

#define TEMP_MIN	18
#define TEMP_MAX	80
#define TEMP_NTEMPS	(sizeof(temp_map) / sizeof(temp_map[0]))

#define NID_MAX		3000

#define SQUARE(x)	((x) * (x))
#define SIGN(x)		((x) == 0 ? 1 : abs(x)/(x))
#define PI		(3.14159265358979323)

#define RM_SELECT	0
#define RM_RENDER	1

#define FOVY		(45.0f)
#define ASPECT		(win_width / (double)win_height)

#define DIMMED_ALPHA	0.2

//#define TILE_TEXTURE	1
#define TEXTURE_SIZE	128.0

#define FONT_WIDTH	12.0
#define FONT_HEIGHT	12.0
#define FONT_TEX_W 	256.0
#define FONT_TEX_H 	16.0

/*
** Determines the texture coordinate by finding the ratio
** between the pixel value given and the max of the three
** possible dimensions. This gives a section of a texture
** without stretching it...
*/
#define NODE_TEXCOORD(X) (1/(MAX3(NODEHEIGHT,NODEWIDTH,NODEDEPTH)/X))

/* How many units of texture coordinates a character displaces */
#define FONT_TEXCOORD_S (1/(FONT_TEX_W/FONT_WIDTH))
#define FONT_TEXCOORD_T (1/(FONT_TEX_H/FONT_HEIGHT))

/* How many pixels a character displaces on a 128x128 tile */
#define FONT_DISPLACE_W ((FONT_WIDTH * NODEDEPTH) / TEXTURE_SIZE)
#define FONT_DISPLACE_H ((FONT_HEIGHT* NODEHEIGHT) / TEXTURE_SIZE)

#define MAX_CHARS 	4
#define FONT_Z_OFFSET ((NODEHEIGHT-((MAX_CHARS+0)*FONT_DISPLACE_W))/2)

#define PNG_FRAMES	4
#define PPM_FRAMES	3

#define FBM_OFF		0
#define FBM_PLAY	1
#define FBM_REC		2

struct vec {
	float		 v_x;
	float		 v_y;
	float		 v_z;
#define v_w v_x
#define v_h v_y
#define v_d v_z
};

struct datasrc {
	void (*ds_physmap)(void);
};

#define DS_FILE		0
#define DS_DB		1

struct vmode {
	int		vm_clip;
	struct vec	vm_ndim;
};

struct fill {
	float		 f_r;
	float		 f_g;
	float		 f_b;
	float		 f_a;
	GLuint		 f_texid;
	GLint		 f_alpha_fmt;
#define f_h f_r
#define f_s f_g
#define f_v f_b
};

struct objhdr {
	int		 oh_ref;
	int		 oh_tref;
};

#define JFL_OWNER	32
#define JFL_NAME	20
#define JFL_QUEUE	10

struct job {
	struct objhdr	 j_oh;
	int		 j_id;
	struct fill	 j_fill;

	char		 j_owner[JFL_OWNER];
	char		 j_jname[JFL_NAME];
	char		 j_queue[JFL_QUEUE];
	int		 j_tmdur;	/* minutes */
	int		 j_tmuse;	/* minutes */
	int		 j_ncpus;
};

struct temp {
	struct objhdr	 t_oh;
	int		 t_cel;
	struct fill	 t_fill;
	char		*t_name;
};

struct fail {
	struct objhdr	 f_oh;
	int		 f_fails;
	struct fill	 f_fill;
	char		*f_name;
};

struct node {
	int		 n_nid;
	struct job	*n_job;
	struct temp	*n_temp;
	struct fail	*n_fail;
	int		 n_state;
	struct fill	*n_fillp;
	struct fill	*n_ofillp;
	int		 n_hide;
	struct vec	 n_wiv;
	struct vec	 n_swiv;
	struct vec	 n_physv;
	struct vec	*n_v;
};

struct state {
	struct vec	 st_v;
	struct vec	 st_lv;
	int		 st_opts;
	int		 st_mode;
	int		 st_vmode;
	int		 st_winspx;
	int		 st_winspy;
	int		 st_winspz;
	int		 st_rf;
#define st_x st_v.v_x
#define st_y st_v.v_y
#define st_z st_v.v_z

#define st_lx st_lv.v_x
#define st_ly st_lv.v_y
#define st_lz st_lv.v_z
};

struct flyby {
	int 		 fb_nid;
	int		 fb_panels;
};

#define FB_OMASK	(OP_LOOPFLYBY | OP_CAPTURE | OP_DISPLAY | OP_STEREO | OP_STOP)
#define FB_PMASK	(PANEL_GOTO | PANEL_CMD | PANEL_FLYBY | PANEL_SS)

#define RF_TEX		(1<<0)
#define RF_PHYSMAP	(1<<1)
#define RF_DATASRC	(1<<2)
#define RF_CLUSTER	(1<<3)
#define RF_SELNODE	(1<<4)
#define RF_PERSPECTIVE	(1<<5)
#define RF_GROUND	(1<<6)
#define RF_INIT		(RF_TEX | RF_PHYSMAP | RF_DATASRC | RF_CLUSTER | \
			 RF_GROUND | RF_SELNODE)

#define OP_TEX		(1<<0)
#define OP_BLEND	(1<<1)
#define OP_WIREFRAME	(1<<2)
#define OP_GROUND	(1<<3)
#define OP_TWEEN	(1<<4)
#define OP_CAPTURE	(1<<5)
#define OP_DISPLAY	(1<<6)
#define OP_GOVERN	(1<<7)
#define OP_LOOPFLYBY	(1<<8)
#define OP_DEBUG	(1<<9)
#define OP_NLABELS	(1<<10)
#define OP_SHOWMODS	(1<<11)
#define OP_WIVMFRAME	(1<<12)
#define OP_PIPES	(1<<13)
#define OP_SELPIPES	(1<<14)
#define OP_STEREO	(1<<15)
#define OP_STOP		(1<<16)

#define SM_JOBS		0
#define SM_FAIL		1
#define SM_TEMP		2

#define VM_PHYSICAL	0
#define VM_WIRED	1
#define VM_WIREDONE	2

struct job_state {
	char		*js_name;
	struct fill	 js_fill;
};

struct uinput {
	struct buf 	  uinp_buf;
	void		(*uinp_callback)(void);
	struct panel	 *uinp_panel;
	int		  uinp_opts;
};

#define UINPO_LINGER	(1<<0)
#define UINPO_DIRTY	(1<<1)

struct pwidget {
	char			 *pw_str;
	struct fill		 *pw_fillp;
	SLIST_ENTRY(pwidget)	  pw_next;
};

struct panel {
	int			  p_id;
	int			  p_dl;
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
	int			  p_nwidgets;
	size_t			  p_maxwlen;
};

#define PANEL_FPS	(1<<0)
#define PANEL_NINFO	(1<<1)
#define PANEL_CMD	(1<<2)
#define PANEL_LEGEND	(1<<3)
#define PANEL_FLYBY	(1<<4)
#define PANEL_GOTO	(1<<5)
#define PANEL_POS	(1<<6)
#define PANEL_SS	(1<<7)
#define PANEL_STATUS	(1<<8)
#define PANEL_MEM	(1<<9)
#define NPANELS		10

#define POPT_REMOVE	(1<<0)			/* being removed */
#define POPT_DIRTY	(1<<1)			/* panel needs redrawn */

TAILQ_HEAD(panels, panel);

struct pinfo {
	void		(*pi_refresh)(struct panel *);
	int		  pi_opts;
	int		  pi_uinpopts;
	void		(*pi_uinpcb)(void);
};

#define PF_UINP		(1<<0)

#define CAMDIR_LEFT	0
#define CAMDIR_RIGHT	1
#define CAMDIR_UP	2
#define CAMDIR_DOWN	3
#define CAMDIR_FORWARD	4
#define CAMDIR_BACK	5

#define TWF_LOOK	(1<<1)
#define TWF_POS		(1<<2)

#define NDF_DONTPUSH	(1<<0)
#define NDF_NOOPTS	(1<<1)

struct dbh {
	union {
		MYSQL dbhu_mysql;
	} dbh_u;
#define dbh_mysql dbh_u.dbhu_mysql
};

struct temp_range {
	struct fill	 m_fill;
	char		*m_name;
};

typedef int (*cmpf_t)(void *, void *);

struct objlist {
	union {
		void		**olu_data;
		struct job	**olu_jobs;
		struct fail	**olu_fails;
		struct temp	**olu_temps;
	}		 ol_udata;
	size_t		 ol_cur;
	size_t		 ol_tcur;
	size_t		 ol_max;
	size_t		 ol_alloc;
	size_t		 ol_incr;
	size_t		 ol_objlen;
	cmpf_t		 ol_eq;
#define ol_data  ol_udata.olu_data
#define ol_jobs  ol_udata.olu_jobs
#define ol_fails ol_udata.olu_fails
#define ol_temps ol_udata.olu_temps
};

/* db.c */
void			 dbh_connect(struct dbh *);
void			 db_physmap(void);

/* cam.c */
void			 cam_move(int, float);
void			 cam_revolve(int);
void			 cam_rotateu(int);
void			 cam_rotatev(int);
void			 cam_update(void);
void			 cam_goto(struct vec *);

/* capture.c */
void			 capture_fb(int);
void 			 begin_capture(int);
void			 end_capture(void);
void			 screenshot(char *, int);

/* draw.c */
void			 draw(void);
void			 draw_node(struct node *, int);
void			 draw_node_pipes(struct vec *);
void			 make_ground(void);
void			 make_cluster(void);

/* flyby.c */
void 			 flyby_begin(int);
void 			 flyby_end(void);
void			 flyby_read(void);
void			 flyby_write(void);
void			 flyby_update(void);

/* key.c */
void			 keyh_flyby(unsigned char, int, int);
void			 keyh_actflyby(unsigned char, int, int);
void			 keyh_uinput(unsigned char, int, int);
void			 keyh_panel(unsigned char, int, int);
void			 keyh_mode(unsigned char, int, int);
void			 keyh_vmode(unsigned char, int, int);
void			 keyh_default(unsigned char, int, int);
void			 spkeyh_default(int, int, int);
void			 spkeyh_actflyby(int, int, int);

/* job.c */
struct job		*job_findbyid(int);

/* load_png.c */
void			 load_texture(void *, GLint, GLenum, GLuint);
void 			*load_png(char *);

/* main.c */
struct node		*node_for_nid(int);
void			 refresh_state(int);
void			 rebuild(int);
void			 select_node(struct node *);
void			 idle_govern(void);
void			 idle(void);
void			 update_textures(void);
void			 restore_textures(void);

/* mouse.c */
void			 m_activeh_default(int, int);
void			 m_activeh_null(int, int);
void			 m_activeh_free(int, int);
void			 m_passiveh_default(int, int);
void			 m_passiveh_null(int, int);
void			 mouseh_default(int, int, int, int);
void			 mouseh_null(int, int, int, int);
void			 sel_record_begin(void);
void			 sel_record_end(void);

/* panel.c */
void			 draw_panels(void);
void			 panel_toggle(int);
void			 panel_tremove(struct panel *);
void			 panel_show(int);
void			 panel_hide(int);
void			 panel_status_addinfo(const char *, ...);
void			 panel_status_setinfo(const char *, ...);
void			 flip_panels(int);

/* parse.c */
void			 parse_jobmap(void);
void			 parse_physmap(void);
void			 parse_failmap(void);
void			 parse_tempmap(void);
void			 parse_mem(void);
void			 obj_batch_start(struct objlist *);
void			 obj_batch_end(struct objlist *);

/* tween.c */
void			 tween_push(int);
void			 tween_pop(int);
void			 tween_probe(float *, float, float, float *, float *);
void			 tween_recalc(float *, float, float, float);

/* uinp.c */
void			 uinpcb_cmd(void);
void			 uinpcb_goto(void);

/* widget.c */
void			 draw_box_outline(const struct vec *, const struct fill *);
void			 draw_box_filled(const struct vec *, const struct fill *);
void			 draw_box_tex(const struct vec *, const struct fill *);
void 			 rgb_contrast(struct fill *c);

extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[];

extern struct job_state	 jstates[];

extern struct objlist	 job_list, temp_list, fail_list;

extern struct fail	 fail_notfound;
extern struct temp	 temp_notfound;

extern int		 total_failures;	/* total shared among all nodes */

extern GLint		 cluster_dl, ground_dl, select_dl;
extern int		 render_mode;
extern int		 font_id;
extern struct uinput	 uinp;
extern int		 spkey;

extern struct state	 st;
extern struct flyby	 fb;
extern long		 fps;
extern struct panels	 panels;
extern struct node	*selnode;
extern struct pinfo	 pinfo[];
extern struct vmode	 vmodes[];
extern struct dbh	 dbh;

extern int		 mode_data_clean;
extern struct vec	 wivstart, wivdim;
extern struct temp_range temp_map[14]; /* XXX */

extern int		 wired_width;			/* XXX: convert to vec */
extern int		 wired_height;
extern int		 wired_depth;

extern float		 tx, tlx, ox, olx;
extern float		 ty, tly, oy, oly;
extern float		 tz, tlz, oz, olz;

extern int		 flyby_mode;
extern int		 capture_mode;

extern int		 win_width;
extern int		 win_height;

extern unsigned long	 vmem;
extern long		 rmem;
