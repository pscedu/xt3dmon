/* $Id$ */

struct uinput {
	struct buf 	  uinp_buf;
	void		(*uinp_callback)(void);
	struct panel	 *uinp_panel;
	int		  uinp_opts;
};

#define UINPO_LINGER	(1<<0)
#define UINPO_DIRTY	(1<<1)

void	 uinpcb_cmd(void);
void	 uinpcb_gotonode(void);
void	 uinpcb_gotojob(void);
void	 uinpcb_login(void);

extern struct uinput	 uinp;

extern char authbuf[];
