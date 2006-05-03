/* $Id$ */

#define SID_LEN		12		/* excluding NUL */

struct session {
	int	 ss_click;
	int	 ss_jobid;
	char	 ss_sid[SID_LEN + 1];
};

void	 serv_init(void);
void	 serv_displayh(void);

extern struct session	*ssp;
extern int		 server_mode;
