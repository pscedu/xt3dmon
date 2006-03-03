/* $Id$ */

#include <ctype.h>

#include "buf.h"
#include "mon.h"
#include "util.h"

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

void
uinpcb_login(void)
{
	struct pinfo *pi;
	struct panel *p;
	char *s, *tobuf;
	size_t siz;

	s = buf_get(&uinp.uinp_buf);

	if ((p = panel_for_id(PANEL_LOGIN)) != NULL) {
		if (p->p_opts & POPT_LOGIN_ATPASS) {
			tobuf = login_pass;
			siz = sizeof(login_pass);

			panel_tremove(p);
			st.st_rf |= RF_DATASRC;
		} else {
			tobuf = login_user;
			siz = sizeof(login_user);

			pi = &pinfo[baseconv(p->p_id) - 1];
			p->p_opts |= POPT_LOGIN_ATPASS | POPT_DIRTY;
			glutKeyboardFunc(gl_keyh_uinput);
			uinp.uinp_panel = p;
			uinp.uinp_opts = pi->pi_uinpopts;
			uinp.uinp_callback = pi->pi_uinpcb;

			login_pass[0] = '\0';
		}
		strncpy(tobuf, s, siz - 1);
		tobuf[siz - 1] = '\0';
	}
}
