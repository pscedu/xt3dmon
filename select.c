/* $Id$ */

#include "cdefs.h"
#include "mon.h"

GLuint	 	 selbuf[1000];

void
sel_begin(void)
{
	GLint viewport[4];

	glSelectBuffer(sizeof(selbuf) / sizeof(selbuf[0]), selbuf);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glRenderMode(GL_SELECT);
	glInitNames();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPickMatrix(lastu, win_height - lastv, 1, 1, viewport);
	/* XXX wrong */
	gluPerspective(FOVY, ASPECT, 0.1, clip);
	glMatrixMode(GL_MODELVIEW);

	obj_batch_start(&glname_list);
}

#define SBI_LEN		0

/*
 * Each GL selecction record consists of the following:
 *	- the number of names in this stack frame,
 *	- the minimum and maximum depths of all
 *	  vertices hit since the previous event, and
 *	- stack contents, bottom first, of which we assume
 *	  consists of only one element.
 */
struct selrec {
	unsigned int sr_names;
	unsigned int sr_minu;
	unsigned int sr_minv;
	unsigned int sr_name;
};

int
selrec_cmp(const void *a, const void *b)
{
	int retval;

	retval = CMP(((struct selrec *)a)->sr_minu,
	    ((struct selrec *)b)->sr_minu);
	if (retval == 0)
		retval = CMP(((struct selrec *)a)->sr_minv,
		    ((struct selrec *)b)->sr_minv);
	return (retval);
}

int
sel_process(int nrecs, int rank, int flags)
{
	struct selrec *sr;
	struct glname *gn;
	GLuint *p;
	int i;

	/* XXX:  sanity-check nrecs? */
	for (i = 0, p = selbuf; i < nrecs; i++, p += 3 + p[SBI_LEN])
		if (p[SBI_LEN] != 1) {
			warnx("selection buffer contains uneven elements");
			return (SP_MISS);
		}

	qsort(selbuf, nrecs, sizeof(*sr), selrec_cmp);

	/*
	 * 2D objects always get placed in the selection buffer,
	 * so the workaround is to pass the parameters of where
	 * they have been drawn on the screen and explicitly check
	 * that the cursor is within the bounds.
	 */
	if ((flags & SPF_2D) == 0) {
		/* Skip 2D records. */
		for (i = 0, sr = (struct selrec *)selbuf; i < nrecs;
		    i++, sr++) {
			gn = glname_list.ol_glnames[sr->sr_name];
			if ((gn->gn_flags & GNF_2D) == 0)
				break;
		}
		rank += i;
	}

	if (rank > nrecs) {
		warnx("requested selection record rank (%d) out of range", rank);
		return (SP_MISS);
	}

	/*
	 * Find the selection record with the given rank.
	 */
	sr = &((struct selrec *)selbuf)[rank];
	gn = glname_list.ol_glnames[sr->sr_name];

	if (flags & SPF_2D) {
		/*
		 * XXX: this code is wrong.
		 * It should loop through and check each
		 * item in selbuf and decrement rank
		 * accordingly for those fitting in the
		 * range, until the correct record is found.
		 */
		 if (lastu < gn->gn_u ||
		     lastu > gn->gn_u + gn->gn_w ||
		     lastv < win_height - gn->gn_v ||
		     lastv > win_height - gn->gn_v + gn->gn_h)
		 	return (SP_MISS);
	}

	gn->gn_cb(gn->gn_id);
	return (gn->gn_id);
}

int
sel_end(void)
{
	obj_batch_end(&glname_list);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glFlush();

	return (glRenderMode(GL_RENDER));
}

/*
 * Obtain a unique GL selection name.
 * Batches of these must be called within a
 * obj_batch_start/obj_batch_end combo.
 */
unsigned int
gsn_get(int id, void (*cb)(int), int flags)
{
	struct glname *gn;
	int cur = glname_list.ol_tcur;

	gn = getobj(&cur, &glname_list);
	gn->gn_id = id;
	gn->gn_cb = cb;
	gn->gn_flags = flags;
	return (cur);
}

/*
 * GL selection callbacks.
 */
void
gscb_panel(int id)
{
	if (spkey == GLUT_ACTIVE_CTRL) {
		/* GL giving us crap. */
		if ((panel_mobile = panel_for_id(id)) != NULL) {
			glutMotionFunc(m_activeh_panel);
			panel_mobile->p_opts |= POPT_MOBILE;
		}
	}
}

void
gscb_row(__unused int a)
{
}

void
gscb_cab(__unused int a)
{
}

void
gscb_cage(__unused int a)
{
}

void
gscb_mod(__unused int a)
{
}

void
gscb_node(int nid)
{
	struct node *n;

	if ((n = node_for_nid(nid)) == NULL)
		return;

	/* spkey = glutGetModifiers(); */
	switch (spkey) {
	case GLUT_ACTIVE_SHIFT:
		sn_add(n);
		break;
	case GLUT_ACTIVE_CTRL:
		sn_del(n);
		break;
	case 0:
		sn_set(n);
		break;
	}
	if (SLIST_EMPTY(&selnodes))
		panel_hide(PANEL_NINFO);
	else
		panel_show(PANEL_NINFO);
	return;
}
