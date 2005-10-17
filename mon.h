/* $Id$ */

#include "compat.h"

#include <sys/queue.h>

#include <mysql.h>

#include "buf.h"
#include "queue.h"
#include "pathnames.h"

#define NROWS		2
#define NCABS		11
#define NCAGES		3
#define NMODS		8
#define NNODES		4

#define WIDIM_WIDTH	11
#define WIDIM_HEIGHT	12
#define WIDIM_DEPTH	16

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
#define CABHEIGHT 	(CAGEHEIGHT * NCAGES + CAGESPACE * (NCAGES - 1));

#define ROWWIDTH	(CABWIDTH * NCABS + CABSPACE * (NCABS - 1))
#define ROWDEPTH	(NODEDEPTH * (NNODES / 2) + \
			    (NODESPACE * (NNODES / 2 - 1)))

#define XCENTER		(NODESPACE + ROWWIDTH / 2)
#define YCENTER		(NODESPACE + (CAGEHEIGHT * NCAGES + \
			    CAGESPACE * (NCAGES - 1)) / 2.0f)
#define ZCENTER		(NODESPACE + (ROWDEPTH * NROWS + \
			    ROWSPACE * (NROWS - 1)) / 2.0f)

#define WIV_SWIDTH	(WIDIM_WIDTH  * st.st_winsp.iv_x)
#define WIV_SHEIGHT	(WIDIM_HEIGHT * st.st_winsp.iv_y)
#define WIV_SDEPTH	(WIDIM_DEPTH  * st.st_winsp.iv_z)
#define WIV_CLIPX	(st.st_winsp.iv_x * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPY	(st.st_winsp.iv_y * vmodes[st.st_vmode].vm_clip)
#define WIV_CLIPZ	(st.st_winsp.iv_z * vmodes[st.st_vmode].vm_clip)

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

#define DEG_TO_RAD(x)	((x) * PI / 180)

#define CMP(a, b) \
	((a) < (b) ? -1 : ((a) == (b) ? 0 : 1))

#define SIGNF(a) \
	(a < 0.0f ? -1.0f : 1.0f)

#define SWAP(a, b, t)		\
	do {			\
		t = a;		\
		a = b;		\
		b = t;		\
	} while (0)

#define FOVY		(45.0f)
#define ASPECT		(win_width / (double)win_height)

#define CM_PNG	0
#define CM_PPM	1

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

/* Stereo display mode types. */
#define STM_NONE	0
#define STM_ACT		1
#define STM_PASV	2

/* Selection processing flags. */
#define SPF_2D		(1<<0)

/* Selection processing return values. */
#define SP_MISS		(-1)

/* Data source providers. */
#define DSP_LOCAL	0
#define DSP_DB		1
#define DSP_REMOTE	2

/* Data sources -- order impacts datasrc[] in ds.c. */
#define DS_TEMP		0
#define DS_PHYS		1
#define DS_JOBS		2
#define DS_BAD		3
#define DS_CHECK	4
#define DS_QSTAT	5
#define DS_MEM		6
#define DS_FAIL		7

/* Data source fetching flags. */
#define DSF_CRIT	(1<<0)
#define DSF_IGN		(1<<1)

struct fvec {
	float		 fv_x;
	float		 fv_y;
	float		 fv_z;
#define fv_w fv_x
#define fv_h fv_y
#define fv_d fv_z

#define fv_r fv_x
#define fv_t fv_y
#define fv_p fv_z
};

struct ivec {
	int		 iv_x;
	int		 iv_y;
	int		 iv_z;
#define iv_w iv_x
#define iv_h iv_y
#define iv_d iv_z

#define iv_u iv_x
#define iv_v iv_y
};

struct physcoord {
	int		 pc_r;
	int		 pc_cb;
	int		 pc_cg;
	int		 pc_m;
	int		 pc_n;
};

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
	GLuint		 f_texid_a;		/* alpha-loaded texid */
#define f_h f_r
#define f_s f_g
#define f_v f_b
};

struct objhdr {
	int		 oh_flags;
};

#define OHF_REF		(1<<0)			/* object is referenced */
#define OHF_TREF	(1<<1)			/* temporarily referenced */
#define OHF_TMP		(1<<2)			/* trashable temporary flag */
#define OHF_OLD		(1<<3)			/* object has been around */
#define OHF_USR1	(1<<4)			/* specific to object */

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

#define JOHF_TOURED	OHF_USR1

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
	struct ivec	 n_wiv;			/* wired view position */
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

#define st_ux st_uv.fv_x
#define st_uy st_uv.fv_y
#define st_uz st_uv.fv_z
};

#define FB_OMASK	(OP_LOOPFLYBY | OP_CAPTURE | OP_DISPLAY | OP_STOP)
#define FB_PMASK	(PANEL_GOTO | PANEL_CMD | PANEL_FLYBY | PANEL_SS)

#define RF_TEX		(1<<0)
#define RF_PHYSMAP	(1<<1)
#define RF_DATASRC	(1<<2)
#define RF_CLUSTER	(1<<3)
#define RF_SELNODE	(1<<4)
#define RF_CAM		(1<<5)
#define RF_GROUND	(1<<6)
#define RF_INIT		(RF_TEX | RF_PHYSMAP | RF_DATASRC | RF_CLUSTER | \
			 RF_GROUND | RF_SELNODE | RF_CAM)

#define EGG_BORG 	(1<<0)
#define EGG_MATRIX 	(1<<1)

#define OP_TEX		(1<<0)
#define OP_WIREFRAME	(1<<1)
#define OP_GROUND	(1<<2)
#define OP_TWEEN	(1<<3)
#define OP_CAPTURE	(1<<4)
#define OP_DISPLAY	(1<<5)
#define OP_GOVERN	(1<<6)
#define OP_LOOPFLYBY	(1<<7)
#define OP_DEBUG	(1<<8)
#define OP_NLABELS	(1<<9)
#define OP_SHOWMODS	(1<<10)
#define OP_WIVMFRAME	(1<<11)
#define OP_PIPES	(1<<12)
#define OP_SELPIPES	(1<<13)
#define OP_STOP		(1<<14)
#define OP_TOURJOB	(1<<15)
#define OP_SKEL		(1<<16)

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
	int			  p_stick;
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
#define PANEL_EGGS	(1<<10) 		/* don't include below (secret) */
#define NPANELS		11

#define POPT_REMOVE	(1<<0)			/* being removed */
#define POPT_DIRTY	(1<<1)			/* panel needs redrawn */
#define POPT_MOBILE	(1<<2)			/* being dragged */

#define PSTICK_TL	1
#define PSTICK_TR	2
#define PSTICK_BL	3
#define PSTICK_BR	4
#define PSTICK_FREE	5

TAILQ_HEAD(panels, panel);

struct pinfo {
	void		(*pi_refresh)(struct panel *);
	int		  pi_opts;
	int		  pi_uinpopts;
	void		(*pi_uinpcb)(void);
};

#define PF_UINP		(1<<0)

#define DIR_LEFT	0
#define DIR_RIGHT	1
#define DIR_UP		2
#define DIR_DOWN	3
#define DIR_FORWARD	4
#define DIR_BACK	5

#define TWF_LOOK	(1<<0)
#define TWF_POS		(1<<1)
#define TWF_ROLL	(1<<2)

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

typedef int (*cmpf_t)(const void *, const void *);

struct objlist {
	union {
		void		**olu_data;
		struct job	**olu_jobs;
		struct fail	**olu_fails;
		struct temp	**olu_temps;
		struct glname	**olu_glnames;
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
#define ol_glnames ol_udata.olu_glnames
};

struct glname {
	struct objhdr	  gn_oh;
	int		  gn_id;	/* Underlying object id. */
	unsigned int 	  gn_name;	/* GL name. */
	int		  gn_flags;
	void		(*gn_cb)(int);

	/* Hack around GL's unwillingness to do 2D selection. */
	int		  gn_u;
	int		  gn_v;
	int		  gn_h;
	int		  gn_w;
};

#define GNF_2D		(1<<0)

struct selnode {
	struct node		 *sn_nodep;
	SLIST_ENTRY(selnode)	  sn_next;
};

SLIST_HEAD(selnodes, selnode);

typedef u_int16_t	 port_t;

struct http_req {
	const char	*htreq_server;
	port_t		 htreq_port;		/* host-byte order. */
	int		 htreq_flags;

	const char	*htreq_method;
	const char	*htreq_version;
	const char	*htreq_url;
	const char	**htreq_extra;		/* NULL-terminate. */
};

struct http_res {
};

/* db.c */
void			 dbh_connect(struct dbh *);
void			 db_physmap(void);
void			 db_jobmap(void);
void			 db_badmap(void);
void			 db_checkmap(void);
void			 db_qstat(void);
void			 db_tempmap(void);
void			 db_failmap(void);

/* callout.c */
void			 cocb_fps(int);
void			 cocb_datasrc(int);
void			 cocb_clearstatus(int);
void			 cocb_tourjob(int);

/* cam.c */
void			 cam_move(int, float);
void			 cam_revolve(struct fvec *, float, float);
void			 cam_rotate(float, float);
void			 cam_roll(float);
void			 cam_goto(struct fvec *);
void			 cam_look(void);

/* capture.c */
void			 capture_frame(int);
void 			 capture_begin(int);
void			 capture_end(void);
void			 capture_snap(const char *, int);
void			 capture_snapfd(int, int);

/* draw.c */
void			 drawh_default(void);
void			 drawh_stereo(void);
void			 drawh_select(void);
void			 draw_node(struct node *, int);
void			 draw_node_pipes(struct fvec *);
void			 make_ground(void);
void			 make_cluster(void);
void			 make_select(void);
float			 snap_to_grid(float, float, float);

/* ds.c */
int			 ds_open(int, int);
void			 ds_refresh(int, int);

/* eggs.c */
void			 egg_borg(void);
void			 egg_matrix(void);

/* flyby.c */
void 			 flyby_begin(int);
void 			 flyby_end(void);
void			 flyby_read(void);
void			 flyby_update(void);
void			 flyby_writeinit(struct state *);
void			 flyby_writeseq(struct state *);
void			 flyby_writepanel(int);
void			 flyby_writeselnode(int);

/* hl.c */
void			 hl_clearall(void);
void			 hl_restoreall(void);
void			 hl_state(int);

/* http.c */
int			 http_open(struct http_req *, struct http_res *);

/* job.c */
struct job		*job_findbyid(size_t);
void			 job_goto(struct job *);
void			 job_hl(struct job *);

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

/* load_png.c */
void 			*png_load(char *, unsigned int *, unsigned int *);
void			 png_write(FILE *, unsigned char *, long, long);

/* main.c */
void			 resizeh(int, int);
void			 refresh_state(int);
void			 rebuild(int);
void			 idleh_govern(void);
void			 idleh_default(void);

/* mouse.c */
void			 m_activeh_default(int, int);
void			 m_activeh_null(int, int);
void			 m_activeh_free(int, int);
void			 m_activeh_panel(int, int);
void			 m_passiveh_default(int, int);
void			 m_passiveh_null(int, int);
void			 mouseh_default(int, int, int, int);
void			 mouseh_null(int, int, int, int);

/* node.c */
struct node		*node_neighbor(struct node *, int, int);
void			 node_physpos(struct node *, struct physcoord *);
void			 node_getmodpos(int, int *, int *);
void			 node_adjmodpos(int, struct fvec *);
struct node		*node_for_nid(int);
void			 node_goto(struct node *);
int			 node_show(struct node *);

/* objlist.c */
void			 obj_batch_start(struct objlist *);
void			 obj_batch_end(struct objlist *);
void			*getobj(const void *, struct objlist *);
void			 getcol(int, size_t, size_t, struct fill *);
void			 getcol_temp(size_t, struct fill *);
int			 job_cmp(const void *, const void *);
int			 fail_cmp(const void *, const void *);
int			 temp_cmp(const void *, const void *);

/* panel.c */
void			 draw_panels(void);
void			 draw_shadow_panels(void);
void			 panel_toggle(int);
void			 panel_tremove(struct panel *);
void			 panel_show(int);
void			 panel_hide(int);
struct panel		*panel_for_id(int);
void			 panel_demobilize(struct panel *);

/* parse.c */
void			 parse_jobmap(int *);
void			 parse_physmap(int *);
void			 parse_failmap(int *);
void			 parse_tempmap(int *);
void			 parse_badmap(int *);
void			 parse_checkmap(int *);
void			 parse_mem(int *);
void			 parse_qstat(int *);

/* select.c */
void			 sel_begin(void);
int			 sel_end(void);
int			 sel_process(int, int, int);

unsigned int		 gsn_get(int, void (*)(int), int);
void			 gscb_node(int);
void			 gscb_panel(int);

/* selnode.c */
void			 sn_toggle(struct node *);
void			 sn_clear(void);
void			 sn_add(struct node *);
void			 sn_insert(struct node *);
int			 sn_del(struct node *);
void			 sn_set(struct node *);
void			 sn_replace(struct selnode *, struct node *);

/* server.c */
void			 serv_init(void);

/* status.c */
void			 status_add(const char *, ...);
void			 status_set(const char *, ...);
void			 status_clear(void);
const char		*status_get(void);

/* tex.c */
void			 tex_load(void);
void			 tex_init(void *, GLint, GLenum, GLuint, GLuint, GLuint);
void			 tex_update(void);
void			 tex_restore(void);
void			 tex_remove(void);

/* tween.c */
void			 tween_push(int);
void			 tween_update(void);
void			 tween_pop(int);
void			 tween_probe(float *, float, float, float *, float *);
void			 tween_recalc(float *, float, float, float);

/* uinp.c */
void			 uinpcb_cmd(void);
void			 uinpcb_goto(void);

/* vec.c */
void			 vec_normalize(struct fvec *);
void			 vec_crossprod(struct fvec *, struct fvec *, struct fvec *);
void			 vec_cart2sphere(struct fvec *, struct fvec *);
void			 vec_sphere2cart(struct fvec *, struct fvec *);
void			 vec_set(struct fvec *, float, float, float);
float			 vec_mag(struct fvec *);

/* widget.c */
void			 draw_box_outline(const struct fvec *, const struct fill *);
void			 draw_box_filled(const struct fvec *, const struct fill *);
void			 draw_box_tex(const struct fvec *, const struct fill *, GLenum);
void 			 rgb_contrast(struct fill *);
void			 hsv_to_rgb(struct fill *);

extern struct node	 nodes[NROWS][NCABS][NCAGES][NMODS][NNODES];
extern struct node	*invmap[];
extern struct node	*wimap[WIDIM_WIDTH][WIDIM_HEIGHT][WIDIM_DEPTH];

extern struct job_state	 jstates[];

extern struct objlist	 job_list, temp_list, fail_list, glname_list;

extern struct fail	 fail_notfound;
extern struct temp	 temp_notfound;

extern int		 total_failures;		/* total among all nodes */

extern int		 cam_dirty;
extern GLint		 cluster_dl, ground_dl, select_dl;
extern int		 font_id;
extern struct uinput	 uinp;
extern int		 spkey;

extern struct state	 st;
extern long		 fps, fps_cnt;
extern struct panels	 panels;
extern struct pinfo	 pinfo[];
extern struct vmode	 vmodes[];
extern struct dbh	 dbh;
extern struct selnodes	 selnodes;
extern size_t		 nselnodes;

extern int		 mode_data_clean;
extern int		 selnode_clean;
extern struct fvec	 wivstart, wivdim;		/* repeat position & dim */
extern struct temp_range temp_map[14]; /* XXX */

extern struct fvec	 tv, tlv, tuv;			/* tween vectors */

extern int		 flyby_mode;
extern int		 capture_mode;
extern int		 stereo_mode;
extern int		 window_ids[2];

extern int		 win_width;
extern int		 win_height;
extern int		 lastu;
extern int		 lastv;

extern unsigned long	 vmem;
extern long		 rmem;

extern float		 clip;
extern int		 eggs;

extern void		(*drawh)(void);
extern void		(*drawh_old)(void);
extern struct panel	*panel_mobile;

extern struct fill	 fill_black;
extern struct fill	 fill_light_blue;
extern int		 dsp;				/* Data source provider. */
