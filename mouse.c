/* $Id$ */

#include "compat.h"

#include <limits.h>
#include <stdlib.h>

#include "cdefs.h"
#include "mon.h"

int	 	 spkey, lastu, lastv;
GLuint	 	 selbuf[1000];
struct panel	*panel_mobile;
void		(*drawh_old)(void);

void
mouseh_null(__unused int button, __unused int state, __unused int u, __unused int v)
{
//	spkey = glutGetModifiers();
}

void
mouseh_default(__unused int button, __unused int state, int u, int v)
{
	spkey = glutGetModifiers();
	if (button == GLUT_LEFT_BUTTON &&
	    state == GLUT_DOWN) {
		drawh_old = drawh;
		drawh = drawh_select;
		glutDisplayFunc(drawh);
	}
	lastu = u;
	lastv = v;

	if (state == GLUT_UP && panel_mobile) {
		panel_demobilize(panel_mobile);
		panel_mobile = NULL;
		glutMotionFunc(m_activeh_default);
	}
}

void
m_activeh_panel(int u, int v)
{
	int du = u - lastu, dv = v - lastv;

	if (abs(du) + abs(dv) <= 1)
		return;
	lastu = u;
	lastv = v;

	panel_mobile->p_u += du;
	panel_mobile->p_v -= dv;
	panel_mobile->p_opts |= POPT_DIRTY;
}

void
revolve_center_cluster(struct fvec *cen)
{
	float dist;

	switch (st.st_vmode) {
	case VM_PHYSICAL:
		cen->fv_x = XCENTER;
		cen->fv_y = YCENTER;
		cen->fv_z = ZCENTER;
		break;
	case VM_WIRED:
	case VM_WIREDONE:
		dist = MAX3(WIV_SWIDTH, WIV_SHEIGHT, WIV_SDEPTH);
		if (st.st_vmode == VM_WIRED)
			dist /= 3.0f;
		cen->fv_x = st.st_x + st.st_lx * dist;
		cen->fv_y = st.st_y + st.st_ly * dist;
		cen->fv_z = st.st_z + st.st_lz * dist;
		break;
	}
}

void
revolve_center_selnode(struct fvec *cen)
{
	struct selnode *sn;

	if (nselnodes == 0) {
		revolve_center_cluster(cen);
		return;
	}
	cen->fv_x = cen->fv_y = cen->fv_z = 0.0f;
	SLIST_FOREACH(sn, &selnodes, sn_next) {
		cen->fv_x += sn->sn_nodep->n_v->fv_x + NODEWIDTH  / 2;
		cen->fv_y += sn->sn_nodep->n_v->fv_y + NODEHEIGHT / 2;
		cen->fv_z += sn->sn_nodep->n_v->fv_z + NODEDEPTH  / 2;
	}
	cen->fv_x /= nselnodes;
	cen->fv_y /= nselnodes;
	cen->fv_z /= nselnodes;
}

void (*revolve_centerf)(struct fvec *) = revolve_center_selnode;

void
m_activeh_default(int u, int v)
{
	int du = u - lastu, dv = v - lastv;

	if (abs(du) + abs(dv) <= 1)
		return;
	lastu = u;
	lastv = v;

	tween_push(TWF_LOOK | TWF_POS);
	if (spkey & GLUT_ACTIVE_CTRL) {
		struct fvec center;

		(*revolve_centerf)(&center);
		cam_revolve(&center, du, dv);
	}
	if (spkey & GLUT_ACTIVE_SHIFT)
		cam_rotate(du, dv);
	tween_pop(TWF_LOOK | TWF_POS);
}

void
m_passiveh_default(int u, int v)
{
	lastu = u;
	lastv = v;
}

void
m_passiveh_null(int u, int v)
{
	lastu = u;
	lastv = v;
}

void
m_activeh_null(int u, int v)
{
	lastu = u;
	lastv = v;
}

void
sel_record_begin(void)
{
	GLint viewport[4];
//	float clip;

//	clip = MIN3(WIV_CLIPX, WIV_CLIPY, WIV_CLIPZ);

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
}

#define SBI_LEN		0
#define SBI_UDST	1
#define SBI_VDST	2
#define SBI_NAMEOFF	3

int
sel_record_process(GLint nrecs, int *done)
{
	GLuint minu, minv, *p, name;
	int i, found, nametype;
	struct panel *pl;
	struct node *n;
	GLuint lastlen;
	GLuint origname = -1;

	*done = 1;
	name = 0; /* gcc */
	found = nametype = 0;
	minu = minv = UINT_MAX;
	/* XXX:  sanity-check nrecs? */
	for (i = 0, p = (GLuint *)selbuf; i < nrecs;
	    i++, lastlen = p[SBI_LEN], p += 3 + p[SBI_LEN]) {
		/*
		 * Each record consists of the following:
		 *	- the number of names in this stack frame,
		 *	- the minimum and maximum depths of all
		 *	  vertices hit since the previous event, and
		 *	- stack contents, bottom first
		 */
		if (p[SBI_LEN] != 1)
			continue;
		if (p[SBI_UDST] <= minu) {
			minu = p[SBI_UDST];
			if (p[SBI_VDST] < minv) {
				/*
				 * XXX: hack to allow 2D objects to
				 * be properly selected.
				 */
				glnametype(p[SBI_NAMEOFF], &origname,
				    &nametype);
				if (nametype == GNAMT_PANEL) {
					if ((pl =
					    panel_for_id(origname)) == NULL)
						continue;
					if (lastu < pl->p_u ||
					    lastu > pl->p_u + pl->p_w ||
					    lastv < win_height - pl->p_v ||
					    lastv > win_height - pl->p_v + pl->p_h)
						continue;
				}
				name = p[SBI_NAMEOFF];
				minv = p[SBI_VDST];
				found = 1;
			}
		}

	}
	if (found) {
		glnametype(name, &origname,
		    &nametype);
		switch (nametype) {
#if 0
		case GNAMT_NODE:
			if ((n = node_for_nid(origname)) != NULL) {
				switch (spkey) {
				case GLUT_ACTIVE_SHIFT:
					sel_add(n);
					break;
				case GLUT_ACTIVE_CTRL:
					sel_del(n);
					break;
				case 0:
					sel_set(n);
					break;
				}
				if (SLIST_EMPTY(&selnodes))
					panel_hide(PANEL_NINFO);
				else
					panel_show(PANEL_NINFO);
			}
			break;
#endif
		case GNAMT_PANEL:
			if (spkey == GLUT_ACTIVE_CTRL) {
				glutMotionFunc(m_activeh_panel);
				/* GL giving us crap. */
				if ((panel_mobile =
				    panel_for_id(origname)) != NULL)
					panel_mobile->p_opts |= POPT_MOBILE;
			}
			break;

		/* anything special needed here? XXX */
		case GNAMT_ROW: printf("row selected: %d\n", origname); *done = 0; break;
		case GNAMT_CAB: printf("cabnet selected: %d\n", origname); *done = 0; break;
		case GNAMT_CAG: printf("cage selected: %d\n", origname); *done = 0; break;
		case GNAMT_MOD: printf("module: %d\n", origname); *done = 0; break;
		case GNAMT_NODE: printf("node: %d\n", origname); break;
		}
	}

	return origname;
}

int
sel_record_end(int *done)
{
	int nrecs;
	int ret = -1;

	*done = 1;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glFlush();

	if ((nrecs = glRenderMode(GL_RENDER)) != 0) {
		ret = sel_record_process(nrecs, done);
	}

	if (done) {
		drawh = drawh_old;
		glutDisplayFunc(drawh);
	}
	/* st.st_rf |= RF_CAM; */
	rebuild(RF_CAM);

	return ret;
}
