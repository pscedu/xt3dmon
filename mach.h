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

#ifndef _MACH_H_
#define _MACH_H_

#include "xmath.h"

struct machine {
	int		m_nidmax;
	struct ivec	m_coredim;
	struct fvec	m_origin;

	int		m_nphysdim;	/* calculated */
	struct fvec	m_center;	/* calculated */
	struct fvec	m_dim;		/* calculated */
};

extern struct machine	machine;
extern int		mach_lineno;

#endif /* _MACH_H_ */
