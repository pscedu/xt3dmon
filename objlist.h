/* $Id$ */

#ifndef _OBJLIST_H_
#define _OBJLIST_H_

struct job;
struct yod;
struct glname;
struct reel;
struct reel_ent;

struct objhdr {
	int	  oh_flags;
};

#define OHF_TMP		(1<<0)		/* trashable temporary flag */
#define OHF_OLD		(1<<1)		/* object has been around */
#define OHF_USR1	(1<<2)		/* specific to object */

struct objlist {
	union {
		void		**olu_data;
		struct job	**olu_jobs;
		struct yod	**olu_yods;
		struct glname	**olu_glnames;
		struct reel	**olu_reels;
		struct reel_ent	**olu_reelents;
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
#define ol_reelents		  ol_udata.olu_reelents
};

void	 obj_batch_start(struct objlist *);
void	 obj_batch_end(struct objlist *);
void	*obj_get(const void *, struct objlist *);

extern struct objlist	 job_list;
extern struct objlist	 yod_list;
extern struct objlist	 glname_list;
extern struct objlist	 reel_list;

#endif /* _OBJLIST_H_ */
