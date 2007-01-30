/* $Id$ */

#include "fill.h"
#include "objlist.h"

#define JFL_OWNER	32
#define JFL_NAME	20
#define JFL_QUEUE	10

struct job {
	struct objhdr	 j_oh;
	int		 j_id;
	struct fill	 j_fill;

	char		 j_owner[JFL_OWNER];
	char		 j_name[JFL_NAME];
	char		 j_queue[JFL_QUEUE];
	int		 j_tmdur;		/* minutes */
	int		 j_tmuse;		/* minutes */
	int		 j_ncpus;
	int		 j_mem;			/* KB */
	int		 j_ncores;
};

#define JOHF_TOURED	OHF_USR1

struct job	*job_findbyid(int, int *);
void		 job_goto(struct job *);
void		 job_hl(struct job *);
int		 job_cmp(const void *, const void *);
