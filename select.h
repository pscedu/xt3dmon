/* $Id$ */

#ifndef _SELECT_H_
#define _SELECT_H_

#include <limits.h>

#include "objlist.h"
#include "xmath.h"

/* Selection processing flags. */
#define SPF_2D		(1<<0)
#define SPF_PROBE	(1<<1)
#define SPF_SQUIRE	(1<<2)
#define SPF_DESQUIRE	(1<<3)
#define SPF_LOOKUP	(1<<4)

struct glname;
typedef void (*gscb_t)(struct glname *, int);

struct glname {
	struct objhdr	  gn_oh;
	unsigned int 	  gn_name;	/* GL name. */
	int		  gn_flags;
	struct fvec	  gn_offv;
	gscb_t		  gn_cb;

	int		  gn_arg_int;	/* Callback "args". */
	int		  gn_arg_int2;
	void		 *gn_arg_ptr;
	void		 *gn_arg_ptr2;
	void		 *gn_arg_ptr3;

	/* Hack around OpenGL's unwillingness to mix 2D/3D selection. */
	int		  gn_u;
	int		  gn_v;
	int		  gn_h;
	int		  gn_w;
};

#define GNF_2D		(1<<0)
#define GNF_PWIDGET	(1<<1)		/* is panel widget */

#define HF_HIDEHELP	0
#define HF_SHOWHELP	1
#define HF_CLRSN	2
#define HF_UPDATE	3
#define HF_PRTSN	4
#define HF_REORIENT	5
#define HF_SUBSN	6

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
struct glname	*sel_process(int, int, int);

struct glname	*wi_shadow(int *, int, const struct fvec *);
struct glname	*phys_shadow(int *, int);

unsigned int	 gsn_get(int, const struct fvec *, gscb_t, int, int, void *, void *);
void		 gscb_miss(struct glname *, int);
void		 gscb_node(struct glname *, int);
void		 gscb_panel(struct glname *, int);
void		 gscb_pw_dir(struct glname *, int);
void		 gscb_pw_dmnc(struct glname *, int);
void		 gscb_pw_dmode(struct glname *, int);
void		 gscb_pw_dscho(struct glname *, int);
void		 gscb_pw_fb(struct glname *, int);
void		 gscb_pw_help(struct glname *, int);
void		 gscb_pw_hlnc(struct glname *, int);
void		 gscb_pw_keyh(struct glname *, int);
void		 gscb_pw_opt(struct glname *, int);
void		 gscb_pw_panel(struct glname *, int);
void		 gscb_pw_pipe(struct glname *, int);
void		 gscb_pw_rt(struct glname *, int);
void		 gscb_pw_snap(struct glname *, int);
void		 gscb_pw_ssmode(struct glname *, int);
void		 gscb_pw_ssvc(struct glname *, int);
void		 gscb_pw_vmode(struct glname *, int);
void		 gscb_pw_wiadj(struct glname *, int);

#endif /* _SELECT_H_ */
