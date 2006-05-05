/* $Id$ */

struct datasrc {
	const char	 *ds_name;
	time_t		  ds_mtime;
	int		  ds_dsp;
	const char	 *ds_lpath;
	const char	 *ds_rpath;
	void		(*ds_parsef)(const struct datasrc *);
	int		  ds_flags;
	void		(*ds_initf)(const struct datasrc *);
	void		(*ds_finif)(const struct datasrc *);
	struct ustream	 *ds_us;
};

/* Data source providers. */
#define DSP_LOCAL	0
#define DSP_REMOTE	1

/* Data source flags. */
#define DSF_AUTO	(1<<0)
#define DSF_FORCE	(1<<1)
#define DSF_READY	(1<<2)
#define DSF_USESSL	(1<<3)

/* Data source fetching flags. */
#define DSFF_CRIT	(1<<0)
#define DSFF_IGN	(1<<1)
#define DSFF_ALERT	(1<<2)

/* Data sources -- order impacts datasrc[] in ds.c. */
#define DS_INV		(-1)
#define DS_NODE		0
#define DS_JOB		1
#define DS_YOD		2
#define DS_RT		3
#define DS_MEM		4
#define DS_SS		5

struct datasrc	*ds_open(int);
struct datasrc	*ds_get(int);
void		 ds_refresh(int, int);

/* Data source cloning. */
int		 dsc_exists(const char *);
void		 dsc_clone(int, const char *);
void		 dsc_load(int, const char *);

/* Data source fetching initialization/finalization. */
void		 dsfi_node(const struct datasrc *);
void		 dsff_node(const struct datasrc *);

int		 st_dsmode(void);

/* parse.c */
void		 parse_job(const struct datasrc *);
void		 parse_mem(const struct datasrc *);
void		 parse_node(const struct datasrc *);
void		 parse_yod(const struct datasrc *);
void		 parse_rt(const struct datasrc *);
void		 parse_ss(const struct datasrc *);

extern int	 dsp;				/* Data source provider. */
