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

struct datasrc {
	char		  ds_name[20];
	time_t		  ds_mtime;
	char		  ds_uri[BUFSIZ];
	void		(*ds_parsef)(struct datasrc *);
	int		  ds_flags;
	void		(*ds_initf)(const struct datasrc *);
	void		(*ds_finif)(const struct datasrc *);
	struct ustream	 *ds_us;
};

/* Data source flags. */
#define DSF_FORCE	(1 << 0)
#define DSF_LIVE	(1 << 1)	/* Real-time data (not interval) */

/* Data source fetching options. */
#define DSFO_CRIT	(1 << 0)
#define DSFO_IGN	(1 << 1)
#define DSFO_ALERT	(1 << 2)
#define DSFO_LIVE	(1 << 3)	/* All live sources are using src */
#define DSFO_SSL	(1 << 4)	/* Use HTTPS */

/* Data sources -- order impacts datasrc[] in ds.c. */
#define DS_INV		(-1)
#define DS_DATA		0
#define NDS		1

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

extern int		 dsfopts;
extern struct datasrc	 datasrcs[];
extern char		 ds_dir[];
extern char		 ds_datasrc[PATH_MAX];
extern char		 ds_browsedir[];
extern char		*ds_liveproto;
