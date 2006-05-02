/* $Id$ */

#include "mon.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "capture.h"
#include "flyby.h"
#include "job.h"
#include "node.h"
#include "nodeclass.h"
#include "panel.h"
#include "queue.h"
#include "selnode.h"
#include "state.h"
#include "uinp.h"
#include "gl.h"
#include "util.h"

char authbuf[BUFSIZ];

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
	int jobid, pos;
	char *s;
	long l;

	s = buf_get(&uinp.uinp_buf);
	l = strtol(s, NULL, 10);
	if (l <= 0 || !isdigit(*s))
		return;
	jobid = (int)l;

	if (job_findbyid(jobid, &pos) == NULL)
		return;
	st.st_hlnc = NSC + pos; /* XXX: check dmode for DM_JOBS */
	st.st_rf |= RF_HLNC;
}

void
uinpcb_eggs(void)
{
	char *cmd;

	cmd = buf_get(&uinp.uinp_buf);

	/* Parse Easter egg keywords =) */
	if (strcmp(cmd, "borg") == 0) {
		st.st_eggs ^= EGG_BORG;
		egg_borg();
	} else if (strcmp(cmd, "matrix") == 0) {
		st.st_eggs ^= EGG_MATRIX;
		egg_matrix();
	}
}

void
uinpcb_login(void)
{
	struct pinfo *pi;
	struct panel *p;
	size_t siz;
	char *s;

	s = buf_get(&uinp.uinp_buf);

	if ((p = panel_for_id(PANEL_LOGIN)) != NULL) {
		siz = sizeof(authbuf) - 1;
		if (p->p_opts & POPT_LOGIN_ATPASS) {
			strncat(authbuf, ":", siz - strlen(authbuf));
			authbuf[siz] = '\0';

			strncat(authbuf, s, siz - strlen(authbuf));
			authbuf[siz] = '\0';

			panel_tremove(p);
			st.st_rf |= RF_DATASRC;

			if (strlen(authbuf) < 4 * sizeof(login_auth) / 3)
				base64_encode(authbuf, login_auth);

			memset(authbuf, 0, sizeof(authbuf));
		} else {
			memset(login_auth, 0, sizeof(login_auth));

			/* For resetting login_auth. */
			if (strcmp(s, "") == 0) {
				panel_tremove(p);
				st.st_rf |= RF_DATASRC;
				return;
			}

			strncpy(authbuf, s, siz);
			authbuf[siz] = '\0';

			pi = &pinfo[baseconv(p->p_id) - 1];
			p->p_opts |= POPT_LOGIN_ATPASS | POPT_DIRTY;
			glutKeyboardFunc(gl_keyh_uinput);
			uinp.uinp_panel = p;
			uinp.uinp_opts = pi->pi_uinpopts;
			uinp.uinp_callback = pi->pi_uinpcb;
		}
	}
}
