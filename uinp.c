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

void
uinpcb_gotonode(void)
{
	struct node *n;
	char *s;
	int nid;
	long l;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 10);
	if (l <= 0 || l > NID_MAX || !isdigit(*s))
		return;
	nid = (int)l;

	if ((n = node_for_nid(nid)) == NULL)
		return;
	sn_add(n);
	node_goto(n);
}

void
uinpcb_gotojob(void)
{
	struct job *j;
	char *s;
	int jid;
	long l;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 10);
	if (l <= 0 || !isdigit(*s))
		return;
	jid = (int)l;

	if ((j = job_findbyid(jid)) == NULL)
		return;
	hl_clearall();
	job_hl(j);

	hlsc = SC_HL_SELJOBS;
	if (flyby_mode == FBM_REC)
		flyby_writehlsc(hlsc);
	st.st_rf |= RF_CLUSTER | RF_SELNODE;
}

void
uinpcb_eggs(void)
{
	char *cmd;

	cmd = buf_get(&uinp.uinp_buf);

	/* Parse Easter egg keywords =) */
	if (strcmp(cmd, "borg") == 0) {
		eggs ^= EGG_BORG;
		egg_borg();
	} else if (strcmp(cmd, "matrix") == 0) {
		eggs ^= EGG_MATRIX;
		egg_matrix();
	}
}
