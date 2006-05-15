/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <stdio.h>

#include "mark.h"
#include "xmath.h"

struct markhd marks;

void
mark_add(struct fvec *fvp)
{
	struct mark *m;

	if ((m = malloc(sizeof(*m))) == NULL)
		err(1, NULL);
	m->m_fv = *fvp;
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
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_POINTS);
	SLIST_FOREACH(m, &marks, m_link)
		glVertex3f(m->m_fv.fv_x, m->m_fv.fv_y, m->m_fv.fv_z);
	glEnd();
}
