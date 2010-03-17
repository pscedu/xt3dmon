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

struct fvec;
struct state;

/* Flyby modes. */
#define FBM_OFF			0
#define FBM_PLAY		1
#define FBM_REC			2

/* Flyby node class highlighting operations -- must stay sync'd to hl.c. */
#define FBHLOP_UNKNOWN		(-1)
#define FBHLOP_XPARENT		0
#define FBHLOP_OPAQUE		1
#define FBHLOP_ALPHAINC		2
#define FBHLOP_ALPHADEC		3
#define FBHLOP_TOGGLEVIS	4

#define FLYBY_DEFAULT		"default"

char	*flyby_set(const char *, int);
void 	 flyby_begin(int);
void 	 flyby_beginauto(void);
void 	 flyby_end(void);
void	 flyby_read(void);
void	 flyby_update(void);
void	 flyby_writeinit(struct state *);
void	 flyby_writeseq(struct state *);
void	 flyby_writepanel(int);
void	 flyby_writeselnode(int, const struct fvec *);
void	 flyby_writehlnc(int, int);
void	 flyby_writecaption(const char *);
void	 flyby_rstautoto(void);
void	 flyby_clear(void);

extern int	flyby_mode;
extern int 	flyby_nautoto;
extern char 	flyby_fn[];
extern char 	flyby_dir[];
