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
uinpcb_goto(void)
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
uinpcb_eggs(void)
{
	char *cmd;

	cmd = buf_get(&uinp.uinp_buf);

	/* Parse Easter egg keywords =) */
	if (strcasecmp(cmd, "borg") == 0) {
		eggs ^= EGG_BORG;
		egg_borg();
	} else if (strcasecmp(cmd, "matrix") == 0) {
		eggs ^= EGG_MATRIX;
		egg_matrix();
	}
}
