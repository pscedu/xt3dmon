/* $Id$ */

#ifndef SLIST_ENTRY
#define SLIST_ENTRY(type) 					\
	struct {						\
		struct type *sle_next;				\
	}
#endif

#ifndef SLIST_HEAD
#define SLIST_HEAD(headtype, type)				\
	struct headtype {					\
		struct type *slh_first;				\
	}
#endif

#ifndef SLIST_FIRST
#define SLIST_FIRST(slh)					\
	((slh)->slh_first)
#endif

#ifndef SLIST_NEXT
#define SLIST_NEXT(elem, memb)					\
	(((elem)->memb).sle_next)
#endif

#ifndef SLIST_INIT
#define SLIST_INIT(slh)						\
	(SLIST_FIRST(slh) = NULL)
#endif

#ifndef SLIST_FOREACH
#define SLIST_FOREACH(slh, elem, memb)				\
	for ((elem) = SLIST_FIRST(slh); ln != NULL;		\
	    (elem) = SLIST_NEXT((elem), memb))
#endif

#ifndef SLIST_INSERT_HEAD
#define SLIST_INSERT_HEAD(slh, elem, memb)			\
	do {							\
		SLIST_NEXT((elem), memb) = SLIST_FIRST(slh);	\
		SLIST_FIRST(slh) = (elem);			\
	} while (0)
#endif

#ifndef SLIST_EMPTY
#define SLIST_EMPTY(slh)					\
	(SLIST_FIRST(slh) == NULL)
#endif
