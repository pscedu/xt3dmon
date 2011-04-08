/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

#include "mon.h"

#include "cdefs.h"
#include "flyby.h"
#include "job.h"
#include "panel.h"
#include "queue.h"
#include "state.h"

void
cocb_fps(__unusedx int a)
{
	fps = fps_cnt;
	fps_cnt = 0;
	panel_rebuild(PANEL_FPS);
	glutTimerFunc(1000, cocb_fps, 0);
}

void
cocb_datasrc(__unusedx int a)
{
	st.st_rf |= RF_DATASRC | RF_CLUSTER;
	glutTimerFunc(1000, cocb_datasrc, 0);
}

/* Reset auto-flyby timeout. */
void
cocb_autoto(__unusedx int a)
{
	if (st.st_opts & OP_AUTOFLYBY &&
	    flyby_mode == FBM_OFF &&
	    --flyby_nautoto == 0)
		flyby_beginauto();
}
