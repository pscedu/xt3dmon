/* $Id$ */

/*
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)queue.h     8.5 (Berkeley) 8/20/94
 */

#include <sys/queue.h>

#ifndef SLIST_ENTRY
#define SLIST_ENTRY(type)						\
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

#undef SLIST_INIT
#define SLIST_INIT(slh)							\
	(SLIST_FIRST(slh) = SLIST_END(slh))

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

#ifndef SLIST_REMOVE_HEAD
#define SLIST_REMOVE_HEAD(slh, memb)					\
	SLIST_FIRST(slh) = SLIST_NEXT(SLIST_FIRST(slh), memb)
#endif

#ifndef LIST_INSERT_BEFORE
#define LIST_INSERT_BEFORE(elem, newelem, memb)				\
	do {								\
		LIST_NEXT((newelem), memb) = (elem);			\
		(newelem)->memb.le_prev = (elem)->memb.le_prev;		\
		*(elem)->memb.le_prev = (newelem);			\
		(elem)->memb.le_prev = &LIST_NEXT((newelem), memb);	\
	} while (0)
#endif

#ifndef LIST_FOREACH
#define LIST_FOREACH(elem, lh, memb)					\
	for ((elem) = LIST_FIRST(lh);					\
	    (elem) != LIST_END(lh);					\
	    (elem) = LIST_NEXT((elem), memb))
#endif

#ifndef LIST_FIRST
#define LIST_FIRST(lh) (lh)->lh_first
#endif

#ifndef LIST_END
#define LIST_END(lh) NULL
#endif

#ifndef LIST_NEXT
#define LIST_NEXT(elem, memb)						\
	((elem)->memb.le_next)
#endif

#ifndef LIST_EMPTY
#define LIST_EMPTY(lh)							\
	(LIST_FIRST(lh) == LIST_END(lh))
#endif

#ifndef TAILQ_HEAD_INITIALIZER
#define TAILQ_HEAD_INITIALIZER(tqh)					\
	{ NULL, &(tqh).tqh_first }
#endif

#ifndef TAILQ_END
#define TAILQ_END(tqh) NULL
#endif

#ifndef TAILQ_LAST
#define TAILQ_LAST(tqh, headtype)					\
	(*(tqh)->tqh_last)
#endif

#ifndef TAILQ_FIRST
#define TAILQ_FIRST(tqh)						\
	((tqh)->tqh_first)
#endif

#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elem, memb)						\
	((elem)->memb.tqe_next)
#endif

#ifndef TAILQ_PREV
#define TAILQ_PREV(elem, headname, memb)				\
	(*(((struct headname *)((elem)->memb.tqe_prev))->tqh_last))
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
