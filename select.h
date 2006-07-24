/* $Id$ */

#include <limits.h>

#include "objlist.h"
#include "xmath.h"

/* Selection processing flags. */
#define SPF_2D		(1<<0)
#define SPF_PROBE	(1<<1)
#define SPF_SQUIRE	(1<<2)
#define SPF_DESQUIRE	(1<<3)

/* Selection processing return values. */
#define SP_MISS		(INT_MIN)

struct glname {
	struct objhdr	  gn_oh;
	int		  gn_id;	/* Underlying object ID. */
	unsigned int 	  gn_name;	/* GL name. */
	int		  gn_flags;
	struct fvec	  gn_offv;
	void		(*gn_cb)(struct glname *, int);

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
#define HF_UPDATE	3
#define HF_PRTSN	4
#define HF_REORIENT	5

#define SWF_NSPX	0
#define SWF_NSPY	1
#define SWF_NSPZ	2
#define SWF_OFFX	3
#define SWF_OFFY	4
#define SWF_OFFZ	5

#define SRF_POS		0
#define SRF_NEG		1
#define SRF_RECOVER	2
#define SRF_FATAL	3
#define SRF_ROUTER	4

#define PWFF_STOPREC	0
#define PWFF_PLAY	1
#define PWFF_REC	2
#define PWFF_CLR	3
#define PWFF_OPEN	4
#define PWFF_NEW	5


void		 sel_begin(void);
int		 sel_end(void);
int		 sel_process(int, int, int);

unsigned int	 gsn_get(int, void (*)(struct glname *, int), int, const struct fvec *);
void		 gscb_miss(struct glname *, int);
void		 gscb_node(struct glname *, int);
void		 gscb_panel(struct glname *, int);
void		 gscb_pw_hlnc(struct glname *, int);
void		 gscb_pw_opt(struct glname *, int);
void		 gscb_pw_panel(struct glname *, int);
void		 gscb_pw_vmode(struct glname *, int);
void		 gscb_pw_dmode(struct glname *, int);
void		 gscb_pw_help(struct glname *, int);
void		 gscb_pw_ssmode(struct glname *, int);
void		 gscb_pw_ssvc(struct glname *, int);
void		 gscb_pw_pipe(struct glname *, int);
void		 gscb_pw_reel(struct glname *, int);
void		 gscb_pw_fb(struct glname *, int);
void		 gscb_pw_wiadj(struct glname *, int);
void		 gscb_pw_rt(struct glname *, int);
void		 gscb_pw_fbcho(struct glname *, int);
void		 gscb_pw_keyh(struct glname *, int);
void		 gscb_pw_dxcho(struct glname *, int);
