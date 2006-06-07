/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <stdio.h>

#include "fill.h"
#include "mark.h"
#include "xmath.h"

struct markhd marks;

void
mark_add(struct fvec *fvp, struct fill *fp)
{
	struct mark *m;

	if ((m = malloc(sizeof(*m))) == NULL)
		err(1, NULL);
	m->m_fv = *fvp;
	m->m_fp = fp;
	SLIST_INSERT_HEAD(&marks, m, m_link);
}

void
mark_clear(void)
{
	struct mark *m;

	while ((m = SLIST_FIRST(&marks)) != SLIST_END(&marks)) {
		SLIST_REMOVE_HEAD(&marks, m_link);
		free(m);
	}
}

void
mark_draw(void)
{
	struct mark *m;

	glPointSize(5.0);
	glBegin(GL_POINTS);
	SLIST_FOREACH(m, &marks, m_link) {
		glColor3f(m->m_fp->f_r, m->m_fp->f_g, m->m_fp->f_b);
		glVertex3f(m->m_fv.fv_x, m->m_fv.fv_y, m->m_fv.fv_z);
	}
	glEnd();
}
