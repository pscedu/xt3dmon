/* $Id$ */

#include <ctype.h>

#include "buf.h"
#include "mon.h"

void
uinpcb_cmd(void)
{
	/* Send command to service node. */
}

void
uinpcb_ss(void)
{
	/* Take screenshot. */
	if (*buf_get(&uinp.uinp_buf) != '\0')
		capture_snap(buf_get(&uinp.uinp_buf), capture_mode);
}

#define GOTO_DIST_PHYS 2.5
#define GOTO_DIST_LOG  2.5

void
uinpcb_goto(void)
{
	struct node *n;
	char *s;
	int nid;
	long l;
	struct fvec cv;
	int mod;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 10);
	if (l <= 0 || l > NID_MAX || !isdigit(*s))
		return;
	nid = (int)l;

	if ((n = node_for_nid(nid)) == NULL)
		return;
	sel_add(n);

	cv = *n->n_v;
	tween_push(TWF_LOOK | TWF_POS);
	switch (st.st_vmode) {
	/* Arranged as nodeid is:
	      -------     -------
	      |     |     |     |
	      |  3  |     |  2  |
	      -------     -------

	   -------     -------
	   |     |     |     |
	   |  0  |     |  1  |
	   -------     -------
	*/
	case VM_PHYSICAL:
		/* Find which side of the blade it's on */
		mod = (n->n_nid + 1) % 4; /* XXX:  wrong, should use address of n */

		st.st_lx = st.st_ly = 0.0;
		cv.fv_y += 0.5 * NODEHEIGHT;

		/* Right side (positive z) */
		if (mod == 2 || mod == 3) {
			/* Change z and look vector */
			cv.fv_z += NODEDEPTH + GOTO_DIST_PHYS;
			st.st_lz = -1.0;
		} else {
			cv.fv_z -= GOTO_DIST_PHYS;
			st.st_lz = 1.0;
		}
		break;
	case VM_WIRED:
	case VM_WIREDONE:
		/* Set to the front where the label will be */
		cv.fv_x -= GOTO_DIST_LOG;
		cv.fv_y += 0.5*NODEHEIGHT;
		cv.fv_z += 0.5*NODEWIDTH;
		st.st_ly = st.st_lz = 0.0;
		st.st_lx = 1.0;
		break;
	}
	cam_goto(&cv);
	tween_pop(TWF_LOOK | TWF_POS);
}
