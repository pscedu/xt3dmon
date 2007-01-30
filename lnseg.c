/* $Id$ */

#include "mon.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "lnseg.h"
#include "fill.h"
#include "draw.h"

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
#if 1
	struct lnseg *ln;

	glLineWidth(0.5);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_LINES);
	SLIST_FOREACH(ln, &lnsegs, ln_link) {
		glVertex3f(ln->ln_sv.fv_x, ln->ln_sv.fv_y, ln->ln_sv.fv_z);
		glVertex3f(ln->ln_ev.fv_x, ln->ln_ev.fv_y, ln->ln_ev.fv_z);
	}
	glEnd();
#else
	struct lnseg *ln;
	struct fill fill_lnseg;

	memset(&fill_lnseg, 0, sizeof(fill_lnseg));
	SLIST_FOREACH(ln, &lnsegs, ln_link) {
		fill_lnseg.f_r = fmod(fill_lnseg.f_r + .2, 1.0);
		fill_lnseg.f_g = fmod(fill_lnseg.f_g + .4, 1.0);
		fill_lnseg.f_b = fmod(fill_lnseg.f_b + .6, 1.0);
		fill_lnseg.f_a = 0.9;

		glPushMatrix();
		glTranslatef(ln->ln_sv.fv_x, ln->ln_sv.fv_y, ln->ln_sv.fv_z);
		draw_cube(&ln->ln_ev, &fill_lnseg, 0);
		glPopMatrix();
	}
#endif
}
