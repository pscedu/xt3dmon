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

#define SID_LEN		12		/* excluding NUL */

struct client_session {
	int				 cs_click;
	int				 cs_jobid;
	char				 cs_sid[SID_LEN + 1];
	struct fvec			 cs_v;
	struct fvec			 cs_lv;
	int				 cs_opts;
	int				 cs_rf;
	int				 cs_dmode;
	int				 cs_vmode;
	struct ivec			 cs_winv;
	struct ivec			 cs_mousev;
	int				 cs_fd;
	int				 cs_hlnc;
	TAILQ_ENTRY(client_session)	 cs_entry;
};

void	 serv_init(void);
void	 serv_displayh(void);

extern struct client_session	*curses;
extern int			 server_mode;
