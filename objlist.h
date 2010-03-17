/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#ifndef _OBJLIST_H_
#define _OBJLIST_H_

struct objhdr {
	int		  oh_flags;
};

#define OHF_TMP		(1<<0)		/* Trashable temporary flag */
#define OHF_OLD		(1<<1)		/* Object has been around */
#define OHF_USR1	(1<<2)		/* Type-specific flag */

struct fnent {
	struct objhdr	  fe_oh;
	char		  fe_name[NAME_MAX];
};

struct objlist {
	void		**ol_data;
	size_t		  ol_cur;
	size_t		  ol_tcur;
	size_t		  ol_max;
	size_t		  ol_alloc;
	size_t		  ol_incr;
	size_t		  ol_objlen;
	cmpf_t		  ol_eq;
};

#define OLE(list, pos, type) ((struct type *)(list).ol_data[pos])

void	 obj_batch_start(struct objlist *);
void	 obj_batch_end(struct objlist *);
void	*obj_get(const void *, struct objlist *);

int	 fe_eq(const void *, const void *);
int	 fe_cmp(const void *, const void *);

extern struct objlist	 ds_list;
extern struct objlist	 dxscript_list;
extern struct objlist	 flyby_list;
extern struct objlist	 glname_list;
extern struct objlist	 job_list;
extern struct objlist	 reel_list;
extern struct objlist	 reelframe_list;
extern struct objlist	 yod_list;

#endif /* _OBJLIST_H_ */
