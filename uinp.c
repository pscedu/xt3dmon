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
		screenshot(buf_get(&uinp.uinp_buf), capture_mode);
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
	struct vec cv;
	int mod;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 0);
	if (l <= 0 || l > NID_MAX || !isdigit(*s))
		return;
	nid = (int)l;

	if ((n = node_for_nid(nid)) == NULL)
		return;

#if 0
	/* Select the node */
	selnode = n;
	st.st_rf = RF_SELNODE;
#endif

	cv = *n->n_v;
	tween_push(TWF_LOOK);

	switch(st.st_vmode) {
		
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
			mod = (n->n_nid + 1) % 4;

			st.st_lx = st.st_ly = 0.0;
			cv.v_y += 0.5 * NODEHEIGHT;

			/* Right side (positive z) */
			if(mod == 2 || mod == 3) {

				/* Change z and look vector */
				cv.v_z += NODEDEPTH + GOTO_DIST_PHYS;
				st.st_lz = -1.0;

			} else {
				cv.v_z -= GOTO_DIST_PHYS;
				st.st_lz = 1.0;
			}

			break;

		case VM_WIRED:
			/* XXX - snap to grid? */
		case VM_WIREDONE:

			/* Set to the front where the label will be */
			cv.v_x -= GOTO_DIST_LOG;
			cv.v_y += 0.5*NODEHEIGHT;
			cv.v_z += 0.5*NODEWIDTH;
			st.st_ly = st.st_lz = 0.0;
			st.st_lx = 1.0;
			break;

		default:
			break;
	}

	tween_pop(TWF_LOOK);
	cam_goto(&cv);
}
