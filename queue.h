/* $Id$ */

#ifndef SLIST_ENTRY
#define SLIST_ENTRY(type) 						\
	struct {							\
		struct type *sle_next;					\
	}
#endif

#ifndef SLIST_HEAD
#define SLIST_HEAD(headtype, type)					\
	struct headtype {						\
		struct type *slh_first;					\
	}
#endif

#ifndef SLIST_END
#define SLIST_END(slh) NULL
#endif

#ifndef SLIST_FIRST
#define SLIST_FIRST(slh)						\
	((slh)->slh_first)
#endif

#ifndef SLIST_NEXT
#define SLIST_NEXT(elem, memb)						\
	(((elem)->memb).sle_next)
#endif

#ifndef SLIST_INIT
#define SLIST_INIT(slh)							\
	(SLIST_FIRST(slh) = NULL)
#endif

#ifndef SLIST_FOREACH
#define SLIST_FOREACH(elem, slh, memb)					\
	for ((elem) = SLIST_FIRST(slh);					\
	    (elem) != SLIST_END(slh);					\
	    (elem) = SLIST_NEXT((elem), memb))
#endif

#ifndef SLIST_INSERT_HEAD
#define SLIST_INSERT_HEAD(slh, elem, memb)				\
	do {								\
		SLIST_NEXT((elem), memb) = SLIST_FIRST(slh);		\
		SLIST_FIRST(slh) = (elem);				\
	} while (0)
#endif

#ifndef SLIST_INSERT_AFTER
#define SLIST_INSERT_AFTER(elem, newelem, memb)				\
	do {								\
		SLIST_NEXT((newelem), memb) =				\
		    SLIST_NEXT((elem), memb);				\
		SLIST_NEXT((elem), memb) = newelem;			\
	} while (0)
#endif

#ifndef SLIST_EMPTY
#define SLIST_EMPTY(slh)						\
	(SLIST_FIRST(slh) == SLIST_END(slh))
#endif

#ifndef SLIST_REMOVE
#define SLIST_REMOVE(slh, elem, type, memb)				\
	do {								\
		struct type *_v;					\
									\
		if (SLIST_FIRST(slh) == elem && elem != NULL)		\
			SLIST_FIRST(slh) =				\
			    SLIST_NEXT(SLIST_FIRST(slh), memb);		\
		else {							\
			SLIST_FOREACH(_v, slh, memb)			\
				if (SLIST_NEXT(_v, memb) == elem)	\
					break;				\
			if (_v != NULL &&				\
			    SLIST_NEXT(_v, memb) != NULL)		\
				SLIST_NEXT(_v, memb) =			\
				    SLIST_NEXT((elem), memb);		\
		}							\
	} while (0)
#endif
