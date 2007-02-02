/* $Id$ */

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
