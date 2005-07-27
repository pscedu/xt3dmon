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

#define NODEWIDTH	(vmodes[st.st_vmode].vm_ndim.fv_w)
#define NODEHEIGHT	(vmodes[st.st_vmode].vm_ndim.fv_h)
#define NODEDEPTH	(vmodes[st.st_vmode].vm_ndim.fv_d)
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

#define WI_WIDTH	(widim.iv_w * st.st_winsp.iv_x)
#define WI_HEIGHT	(widim.iv_h * st.st_winsp.iv_y)
#define WI_DEPTH	(widim.iv_d * st.st_winsp.iv_z)
#define WI_CLIPX	(st.st_winsp.iv_x * vmodes[st.st_vmode].vm_clip)
#define WI_CLIPY	(st.st_winsp.iv_y * vmodes[st.st_vmode].vm_clip)
#define WI_CLIPZ	(st.st_winsp.iv_z * vmodes[st.st_vmode].vm_clip)

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
#define NODE_TEXCOORD(X) (1 / (MAX3(NODEHEIGHT, NODEWIDTH, NODEDEPTH) / X))

/* How many units of texture coordinates a character displaces */
#define FONT_TEXCOORD_S (1 / (FONT_TEX_W / FONT_WIDTH))
#define FONT_TEXCOORD_T (1 / (FONT_TEX_H / FONT_HEIGHT))

/* How many pixels a character displaces on a 128x128 tile */
#define FONT_DISPLACE_W ((FONT_WIDTH * NODEDEPTH) / TEXTURE_SIZE)
#define FONT_DISPLACE_H ((FONT_HEIGHT * NODEHEIGHT) / TEXTURE_SIZE)

#define MAX_CHARS 	4
#define FONT_Z_OFFSET	((NODEHEIGHT - ((MAX_CHARS + 0) * FONT_DISPLACE_W)) / 2)

#define PNG_FRAMES	4
#define PPM_FRAMES	3

/* HSV constants. */
#define HUE_MIN 0
#define HUE_MAX 360
#define SAT_MIN 0.3
#define SAT_MAX 1.0
#define VAL_MIN 0.5
#define VAL_MAX 1.0

/* Flyby modes. */
#define FBM_OFF		0
#define FBM_PLAY	1
#define FBM_REC		2

/* GL name types. */
#define GNAMT_NODE	1	/* must start at 1 */
#define GNAMT_PANEL	2

struct fvec {
	float		 fv_x;
	float		 fv_y;
	float		 fv_z;
#define fv_w fv_x
#define fv_h fv_y
#define fv_d fv_z
};

struct ivec {
	int		 iv_x;
	int		 iv_y;
	int		 iv_z;
#define iv_w iv_x
#define iv_h iv_y
#define iv_d iv_z
};

struct datasrc {
	void (*ds_physmap)(void);
};

#define DS_FILE		0
#define DS_DB		1

struct vmode {
	int		 vm_clip;
	struct fvec	 vm_ndim;		/* node dimensions */
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
	int		 oh_tref;		/* temporarily referenced */
	int		 oh_flags;
};

#define OHF_REF		(1<<0)			/* object is referenced */
#define OHF_TREF	(1<<1)			/* temporarily referenced */
#define OHF_SEL		(1<<2)			/* selected */

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
	int		 j_tmdur;		/* minutes */
	int		 j_tmuse;		/* minutes */
	int		 j_ncpus;
};

struct temp {
	struct objhdr	 t_oh;
	int		 t_cel;			/* degrees celcius */
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
	int		 n_flags;
	struct fvec	 n_wiv;			/* wired view position */
	struct fvec	 n_swiv;		/* scaled */
	struct fvec	 n_physv;
	struct fvec	*n_v;
};

#define NF_HIDE		(1<<0)
#define NF_SEL		(1<<1)

struct state {
	struct fvec	 st_v;			/* camera position */
	struct fvec	 st_lv;			/* camera look direction */
	struct fvec	 st_uv;			/* up direction */
	int		 st_opts;
	int		 st_mode;		/* data mode */
	int		 st_vmode;		/* view mode */
	struct ivec	 st_winsp;		/* wired node spacing */
	int		 st_rf;
#define st_x st_v.fv_x
#define st_y st_v.fv_y
#define st_z st_v.fv_z

#define st_lx st_lv.fv_x
#define st_ly st_lv.fv_y
#define st_lz st_lv.fv_z
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
#define POPT_MOBILE	(1<<2)			/* being dragged */

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

struct selnode {
	struct node		*sn_nodep;
	SLIST_ENTRY(selnode)	 sn_next;
};

SLIST_HEAD(selnodes, selnode) selnodes;

/* db.c */
void			 dbh_connect(struct dbh *);
void			 db_physmap(void);

/* cam.c */
void			 cam_move(int, float);
void			 cam_revolve(int);
void			 cam_rotateu(int);
void			 cam_rotatev(int);
void			 cam_update(void);
void			 cam_goto(struct fvec *);

/* capture.c */
void			 capture_fb(int);
void 			 begin_capture(int);
void			 end_capture(void);
void			 screenshot(char *, int);

/* draw.c */
void			 draw(void);
void			 draw_node(struct node *, int);
void			 draw_node_pipes(struct fvec *);
void			 make_ground(void);
void			 make_cluster(void);
void			 make_select(void);
float			 snap_to_grid(float, float, float);

/* flyby.c */
void 			 flyby_begin(int);
void 			 flyby_end(void);
void			 flyby_read(void);
void			 flyby_update(void);
void			 flyby_writeinit(struct state *);
void			 flyby_writeseq(struct state *);
void			 flyby_writepanel(int);
void			 flyby_writeselnode(int);

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
void			 idle_govern(void);
void			 idle(void);
void			 update_textures(void);
void			 restore_textures(void);
unsigned int		 mkglname(unsigned int, int);
void			 glnametype(unsigned int, unsigned int *, int *);

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

/* objlist.c */
void			 obj_batch_start(struct objlist *);
void			 obj_batch_end(struct objlist *);
void			*getobj(void *, struct objlist *);
void			 getcol(int, size_t, struct fill *);
void			 getcol_temp(int, struct fill *);
int			 job_cmp(const void *, const void *);
int			 fail_cmp(const void *, const void *);
int			 temp_cmp(const void *, const void *);

/* panel.c */
void			 draw_panels(void);
void			 panel_toggle(int);
void			 panel_tremove(struct panel *);
void			 panel_show(int);
void			 panel_hide(int);
void			 panel_status_addinfo(const char *, ...);
void			 panel_status_setinfo(const char *, ...);
struct panel		*panel_for_id(int);
void			 flip_panels(int);

/* parse.c */
void			 parse_jobmap(void);
void			 parse_physmap(void);
void			 parse_failmap(void);
void			 parse_tempmap(void);
void			 parse_mem(void);

/* select.c */
void			 sel_toggle(struct node *);
void			 sel_clear(void);
void			 sel_add(struct node *);
void			 sel_insert(struct node *);
int			 sel_del(struct node *);
void			 sel_set(struct node *);

/* tween.c */
void			 tween_push(int);
void			 tween_pop(int);
void			 tween_probe(float *, float, float, float *, float *);
void			 tween_recalc(float *, float, float, float);

/* uinp.c */
void			 uinpcb_cmd(void);
void			 uinpcb_goto(void);

/* vec.c */
void			 vec_normalize(struct fvec *);

/* widget.c */
void			 draw_box_outline(const struct fvec *, const struct fill *);
void			 draw_box_filled(const struct fvec *, const struct fill *);
void			 draw_box_tex(const struct fvec *, const struct fill *, GLenum);
void 			 rgb_contrast(struct fill *);
void			 HSV2RGB(struct fill *);

extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[];

extern struct job_state	 jstates[];

extern struct objlist	 job_list, temp_list, fail_list;

extern struct fail	 fail_notfound;
extern struct temp	 temp_notfound;

extern int		 total_failures;		/* total among all nodes */

extern GLint		 cluster_dl, ground_dl, select_dl;
extern int		 render_mode;
extern int		 font_id;
extern struct uinput	 uinp;
extern int		 spkey;

extern struct state	 st;
extern long		 fps;
extern struct panels	 panels;
extern struct pinfo	 pinfo[];
extern struct vmode	 vmodes[];
extern struct dbh	 dbh;
extern struct selnodes	 selnodes;
extern size_t		 nselnodes;

extern struct ivec	 widim;				/* wired cluster dimensions */
extern int		 mode_data_clean;
extern int		 selnode_clean;
extern struct fvec	 wivstart, wivdim;		/* repeat position & dim */
extern struct temp_range temp_map[14]; /* XXX */

extern struct fvec	 tv, tlv;

extern int		 flyby_mode;
extern int		 capture_mode;

extern int		 win_width;
extern int		 win_height;

extern unsigned long	 vmem;
extern long		 rmem;
