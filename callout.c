/* $Id$ */

#include "mon.h"

#include "cdefs.h"
#include "flyby.h"
#include "job.h"
#include "panel.h"
#include "queue.h"
#include "state.h"

void
cocb_fps(__unused int a)
{
#if 0
	/*
	 * If the frame rate over the last second was
	 * significantly lower than the second before,
	 * increase the tweening interval and decrease
	 * the tweening threshold so the animation
	 * isn't so slow.
	 */
	if (fps_cnt < fps) {
		tween_intv *= fps / fps_cnt;
		tween_thres *= fps_cnt / fps;
	}
#endif

	fps = fps_cnt;
	fps_cnt = 0;
	panel_rebuild(PANEL_FPS);
	glutTimerFunc(1000, cocb_fps, 0);
}

void
cocb_datasrc(__unused int a)
{
	st.st_rf |= RF_DATASRC | RF_CLUSTER;
	glutTimerFunc(1000, cocb_datasrc, 0);
}

/* Reset auto-flyby timeout. */
void
cocb_autoto(__unused int a)
{
	if (st.st_opts & OP_AUTOFLYBY &&
	    flyby_mode == FBM_OFF &&
	    --flyby_nautoto == 0)
		flyby_beginauto();
}
