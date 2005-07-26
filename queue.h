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

#ifndef SLIST_FOREACH_PREVPTR
#define SLIST_FOREACH_PREVPTR(elem, prev, slh, memb)			\
	for ((prev) = &SLIST_FIRST(slh);				\
	    ((elem) = *(prev)) != SLIST_END(slh);			\
	    (prev) = &SLIST_NEXT((sn), memb))
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

#ifndef SLIST_REMOVE_NEXT
#define SLIST_REMOVE_NEXT(slh, elem, memb)				\
	do {								\
		SLIST_NEXT((elem), memb) =				\
		    SLIST_NEXT(SLIST_NEXT((elem), memb), memb);		\
	} while (0)
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

#ifndef TAILQ_END
#define TAILQ_END(tqh) NULL
#endif

#ifndef TAILQ_FIRST
#define TAILQ_FIRST(tqh)						\
	((tqh)->tqh_first)
#endif

#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elem, memb)						\
	(((elem)->memb).tqe_next)
#endif

#ifndef TAILQ_EMPTY
#define TAILQ_EMPTY(tqh)						\
	(TAILQ_FIRST(tqh) == TAILQ_END(tqh))
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(elem, tqh, memb)					\
	for ((elem) = TAILQ_FIRST(tqh);					\
	    (elem) != TAILQ_END(tqh);					\
	    (elem) = TAILQ_NEXT((elem), memb))
#endif

#ifndef CIRCLEQ_FIRST
#define CIRCLEQ_FIRST(cqh)						\
	((cqh)->cqh_first)
#endif

#ifndef CIRCLEQ_END
#define CIRCLEQ_END(cqh)						\
	((cqh)->cqh_last)
#endif

#ifndef CIRCLEQ_NEXT
#define CIRCLEQ_NEXT(elem, memb)					\
	(((elem)->memb).cqe_next)
#endif

#ifndef CIRCLEQ_FOREACH
#define CIRCLEQ_FOREACH(elem, cqh, memb)				\
	for ((elem) = CIRCLEQ_FIRST(cqh);				\
	    (elem) != CIRCLEQ_END(cqh);					\
	    (elem) = CIRCLEQ_NEXT((elem), memb))
#endif
