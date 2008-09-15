/* $Id$ */

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
