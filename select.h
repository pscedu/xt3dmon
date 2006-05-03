/* $Id$ */

#include "objlist.h"

/* Selection processing flags. */
#define SPF_2D		(1<<0)
#define SPF_PROBE	(1<<1)

/* Selection processing return values. */
#define SP_MISS		(INT_MIN)

struct glname {
	struct objhdr	  gn_oh;
	int		  gn_id;	/* Underlying object ID. */
	unsigned int 	  gn_name;	/* GL name. */
	int		  gn_flags;
	void		(*gn_cb)(int, int);

	/* Hack around GL's unwillingness to do 2D selection. */
	int		  gn_u;
	int		  gn_v;
	int		  gn_h;
	int		  gn_w;
};

#define GNF_2D		(1<<0)

#define HF_HIDEHELP	0
#define HF_SHOWHELP	1
#define HF_CLRSN	2

void		 sel_begin(void);
int		 sel_end(void);
int		 sel_process(int, int, int);

unsigned int	 gsn_get(int, void (*)(int, int), int);
void		 gscb_miss(int, int);
void		 gscb_node(int, int);
void		 gscb_panel(int, int);
void		 gscb_pw_hlnc(int, int);
void		 gscb_pw_opt(int, int);
void		 gscb_pw_panel(int, int);
void		 gscb_pw_vmode(int, int);
void		 gscb_pw_dmode(int, int);
void		 gscb_pw_help(int, int);
void		 gscb_pw_ssmode(int, int);
void		 gscb_pw_ssvc(int, int);
void		 gscb_pw_pipe(int, int);
void		 gscb_pw_reel(int, int);
