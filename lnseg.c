/* $Id$ */

#include <err.h>
#include <stdlib.h>

#include "lnseg.h"

struct lnseghd lnsegs;

void
lnseg_add(struct fvec *sv, struct fvec *ev)
{
	struct lnseg *ln;

	if ((ln = malloc(sizeof(*ln))) == NULL)
		err(1, NULL);
	ln->ln_sv = *sv;
	ln->ln_ev = *ev;
	SLIST_INSERT_HEAD(&lnsegs, ln, ln_link);
}

void
lnseg_clear(void)
{
	struct lnseg *ln;

	while ((ln = SLIST_FIRST(&lnsegs)) != SLIST_END(&lnsegs)) {
		SLIST_REMOVE_HEAD(&lnsegs, ln_link);
		free(ln);
	}
}

void
lnseg_draw(void)
{
	struct lnseg *ln;

	glLineWidth(0.5);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_LINES);
	SLIST_FOREACH(ln, &lnsegs, ln_link) {
		glVertex3f(ln->ln_sv.fv_x, ln->ln_sv.fv_y, ln->ln_sv.fv_z);
		glVertex3f(ln->ln_ev.fv_x, ln->ln_ev.fv_y, ln->ln_ev.fv_z);
	}
	glEnd();
}
