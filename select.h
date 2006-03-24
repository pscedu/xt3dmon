/* $Id$ */

#include "objlist.h"

/* Selection processing flags. */
#define SPF_2D	(1<<0)

/* Selection processing return values. */
#define SP_MISS	(-1)

struct glname {
	struct objhdr	  gn_oh;
	int		  gn_id;	/* Underlying object ID. */
	unsigned int 	  gn_name;	/* GL name. */
	int		  gn_flags;
	void		(*gn_cb)(int);

	/* Hack around GL's unwillingness to do 2D selection. */
	int		  gn_u;
	int		  gn_v;
	int		  gn_h;
	int		  gn_w;
};

#define GNF_2D	(1<<0)

void		 sel_begin(void);
int		 sel_end(void);
int		 sel_process(int, int, int);

unsigned int	 gsn_get(int, void (*)(int), int);
void		 gscb_node(int);
void		 gscb_panel(int);
void		 gscb_pw_hlnc(int);
void		 gscb_pw_hlall(int);
void		 gscb_pw_opt(int);
void		 gscb_pw_panel(int);
