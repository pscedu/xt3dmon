/* $Id$ */

#ifndef _OBJLIST_H_
#define _OBJLIST_H_

struct job;
struct yod;
struct glname;
struct reel;
struct fnent;
struct color;

struct objhdr {
	int	  oh_flags;
};

#define OHF_TMP		(1<<0)		/* trashable temporary flag */
#define OHF_OLD		(1<<1)		/* object has been around */
#define OHF_USR1	(1<<2)		/* specific to object */

struct fnent {
	struct objhdr	fe_oh;
	char		fe_name[NAME_MAX];
};

struct objlist {
	union {
		void		**olu_data;
		struct job	**olu_jobs;
		struct yod	**olu_yods;
		struct glname	**olu_glnames;
		struct reel	**olu_reels;
		struct fnent	**olu_fnents;
		struct color	**olu_colors;
	}	 		  ol_udata;
	size_t	 		  ol_cur;
	size_t	 		  ol_tcur;
	size_t	 		  ol_max;
	size_t	 		  ol_alloc;
	size_t	 		  ol_incr;
	size_t	 		  ol_objlen;
	cmpf_t	 		  ol_eq;
#define ol_data			  ol_udata.olu_data
#define ol_jobs			  ol_udata.olu_jobs
#define ol_yods			  ol_udata.olu_yods
#define ol_glnames		  ol_udata.olu_glnames
#define ol_reels		  ol_udata.olu_reels
#define ol_fnents		  ol_udata.olu_fnents
#define ol_colors		  ol_udata.olu_colors
};

void	 obj_batch_start(struct objlist *);
void	 obj_batch_end(struct objlist *);
void	*obj_get(const void *, struct objlist *);

int	 fe_eq(const void *, const void *);
int	 fe_cmp(const void *, const void *);

extern struct objlist	 job_list;
extern struct objlist	 yod_list;
extern struct objlist	 glname_list;
extern struct objlist	 reel_list;
extern struct objlist	 flyby_list;
extern struct objlist	 dxscript_list;
extern struct objlist	 col_list;

#endif /* _OBJLIST_H_ */
