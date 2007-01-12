/* $Id$ */

struct datasrc {
	char		  ds_name[20];
	time_t		  ds_mtime;
	char		  ds_uri[BUFSIZ];
	void		(*ds_parsef)(const struct datasrc *);
	int		  ds_flags;
	void		(*ds_initf)(const struct datasrc *);
	void		(*ds_finif)(const struct datasrc *);
	struct ustream	 *ds_us;
};

/* Data source flags. */
#define DSF_FORCE	(1<<0)
#define DSF_LIVE	(1<<1)		/* Real-time data (not interval). */

/* Data source fetching options. */
#define DSFO_CRIT	(1<<0)
#define DSFO_IGN	(1<<1)
#define DSFO_ALERT	(1<<2)
#define DSFO_LIVE	(1<<3)		/* All live-able sources are using src. */

/* Data sources -- order impacts datasrc[] in ds.c. */
#define DS_INV		(-1)
#define DS_NODE		0
#define DS_JOB		1
#define DS_YOD		2
#define DS_RT		3
#define DS_SS		4
#define NDS		5

void		 ds_setlive(void);
void		 ds_seturi(int, const char *);
char		*ds_set(const char *, int);
struct datasrc	*ds_open(int);
void		 ds_refresh(int, int);

/* Data source cloning. */
int		 dsc_exists(const char *);
void		 dsc_clone(int, const char *);
int		 dsc_load(int, const char *);

/* Data source fetching initialization/finalization. */
void		 dsfi_node(const struct datasrc *);
void		 dsff_node(const struct datasrc *);

int		 st_dsmode(void);

int		 gss_valid(const char *);
void		 gss_build_auth(struct ustream *);

extern int	 	 dsfopts;
extern struct datasrc	 datasrcs[];
extern char		 ds_dir[];
extern char		 ds_browsedir[];
extern char		*ds_liveproto;
