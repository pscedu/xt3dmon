/* $Id$ */

/*
 * General notes for the panel-handling code.
 *
 *	(*) Panels are redrawn when the POPT_DIRTY flag is set in their
 *	    structure.  This means only that GL needs to re-render
 *	    the panel.  Panels must determine on their on, in their
 *	    refresh function, when their information itself is
 *	    "dirty."
 */

#include "compat.h"

#include <sys/stat.h>

#include <err.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buf.h"
#include "cdefs.h"
#include "mon.h"
#include "queue.h"

#define LETTER_HEIGHT	13
#define LETTER_WIDTH	8
#define PANEL_PADDING	3
#define PANEL_BWIDTH	(2.0f)
#define PWIDGET_LENGTH	15
#define PWIDGET_HEIGHT	LETTER_HEIGHT
#define PWLABEL_LENGTH	(win_width / 4 / 2 - PWIDGET_PADDING)
#define PWIDGET_PADDING	3

void panel_refresh_fps(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_cmd(struct panel *);
void panel_refresh_legend(struct panel *);
void panel_refresh_flyby(struct panel *);
void panel_refresh_goto(struct panel *);
void panel_refresh_pos(struct panel *);
void panel_refresh_ss(struct panel *);
void panel_refresh_status(struct panel *);
void panel_refresh_mem(struct panel *);
void panel_refresh_eggs(struct panel *);
void panel_refresh_date(struct panel *);

int  panel_blink(struct timeval *, char **, int, int *, long);

void uinpcb_ss(void);
void uinpcb_eggs(void);

struct pinfo pinfo[] = {
	{ panel_refresh_fps,	0,	 0,		 NULL },
	{ panel_refresh_ninfo,	0,	 0,		 NULL },
	{ panel_refresh_cmd,	PF_UINP, UINPO_LINGER,	 uinpcb_cmd },
	{ panel_refresh_legend,	0,	 0,		 NULL },
	{ panel_refresh_flyby,	0,	 0,		 NULL },
	{ panel_refresh_goto,	PF_UINP, 0,		 uinpcb_goto },
	{ panel_refresh_pos,	0,	 0,		 NULL },
	{ panel_refresh_ss,	PF_UINP, 0,		 uinpcb_ss },
	{ panel_refresh_status,	0,	 0,		 NULL },
	{ panel_refresh_mem,	0,	 0,		 NULL },
	{ panel_refresh_eggs,	PF_UINP, 0,		 uinpcb_eggs },
	{ panel_refresh_date,	PF_XPARENT, 0,		 NULL }
};

#define PVOFF_TL 0
#define PVOFF_TR 1
#define PVOFF_BL 2
#define PVOFF_BR 3
#define NPVOFF 4

struct panels	 panels;
int		 panel_offset[NPVOFF];
int		 mode_data_clean;
int		 selnode_clean;

int
baseconv(int n)
{
	unsigned int i;

	for (i = 0; i < sizeof(n) * 8; i++)
		if (n & (1 << i))
			return (i + 1);
	return (0);
}

void
panel_free(struct panel *p)
{
	struct pwidget *pw, *nextp;

	if (p->p_dl[WINID_LEFT])
		glDeleteLists(p->p_dl[WINID_LEFT], 1);
	if (p->p_dl[WINID_RIGHT])
		glDeleteLists(p->p_dl[WINID_RIGHT], 1);
	TAILQ_REMOVE(&panels, p, p_link);
	for (pw = SLIST_FIRST(&p->p_widgets);
	    pw != SLIST_END(&p->p_widgets);
	    pw = nextp) {
		nextp = SLIST_NEXT(pw, pw_next);
		free(pw);
	}
	free(p->p_str);
	free(p);
}

void
draw_shadow_panels(void)
{
	struct glname *gn;
	struct panel *p;
	unsigned int name;

	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, win_width, 0.0, win_height);

	TAILQ_FOREACH(p, &panels, p_link) {
		name = gsn_get(p->p_id, gscb_panel, GNF_2D);
		gn = getobj(&name, &glname_list);
		gn->gn_u = p->p_u;
		gn->gn_v = p->p_v;
		gn->gn_h = p->p_h;
		gn->gn_w = p->p_w;

		glPushMatrix();
		glPushName(name);
		glBegin(GL_POLYGON);
		glVertex2d(p->p_u,		p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
		glVertex2d(p->p_u,		p->p_v - p->p_h);
		glEnd();
		glPopName();
		glPopMatrix();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

/*
 * The panel drawing API naming is horrible but is consistent:
 * draw_panel is the low-level function that draws an actual panel.
 * panel_draw is the high-level panel function that does what is
 * necessary to put a panel on the screen.
 */
void
draw_panel(struct panel *p, int toff)
{
	int npw, uoff, voff;
	struct pwidget *pw;
	char *s;

	/* Save state and set things up for 2D. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, win_width, 0.0, win_height);

	glColor4f(p->p_fill.f_r, p->p_fill.f_g, p->p_fill.f_b, p->p_fill.f_a);

	/* Panel content. */
	voff = p->p_v - toff;
	uoff = p->p_u + toff;
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n' || s == p->p_str) {
			voff -= LETTER_HEIGHT;
			if (voff < p->p_v - p->p_h)
				break;
			glRasterPos2d(uoff, voff);
			if (*s == '\n')
				continue;
		}
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
	}

	npw = 0;
	uoff += p->p_w / 2;
	voff += PWIDGET_HEIGHT; /* first loop cuts into this */
	SLIST_FOREACH(pw, &p->p_widgets, pw_next) {
		struct fill *fp = pw->pw_fillp;

		uoff += p->p_w / 2 * (npw % 2 ? 1 : -1);
		if (npw % 2 == 0)
			voff -= PWIDGET_HEIGHT + PWIDGET_PADDING;
		if (voff - PWIDGET_HEIGHT < p->p_v - p->p_h)
			break;

		/* Draw widget background. */
		glBegin(GL_POLYGON);
		glColor4f(fp->f_r, fp->f_g, fp->f_b, 1.0f /* XXX */);
		glVertex2d(uoff + 1,			voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff - PWIDGET_HEIGHT + 1);
		glVertex2d(uoff + 1,			voff - PWIDGET_HEIGHT + 1);
		glEnd();

		/* Draw widget border. */
		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		glColor4f(0.00f, 0.00f, 0.00f, 1.00f);
		glVertex2d(uoff,			voff);
		glVertex2d(uoff + PWIDGET_LENGTH + 1,	voff);
		glVertex2d(uoff + PWIDGET_LENGTH,	voff - PWIDGET_HEIGHT);
		glVertex2d(uoff,			voff - PWIDGET_HEIGHT);
		glEnd();

		glColor4f(p->p_fill.f_r, p->p_fill.f_g, p->p_fill.f_b, p->p_fill.f_a);
		glRasterPos2d(uoff + PWIDGET_LENGTH + PWIDGET_PADDING,
		    voff - PWIDGET_HEIGHT + 3);
		for (s = pw->pw_str; *s != '\0' &&
		    (s - pw->pw_str) * LETTER_WIDTH + PWIDGET_LENGTH +
		    PWIDGET_PADDING * 4 < p->p_w / 2; s++)
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);

		/*
		 * We may have to break early because of the way
		 * persistent memory allocations are performed.
		 */
		if (++npw >= p->p_nwidgets)
			break;
	}

	if ((pinfo[baseconv(p->p_id) - 1].pi_opts & PF_XPARENT) == 0) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/* Draw background. */
		glBegin(GL_POLYGON);
		glColor4f(0.4, 0.6, 0.8, 0.8);
		glVertex2d(p->p_u,		p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
		glVertex2d(p->p_u,		p->p_v - p->p_h);
		glEnd();

		glDisable(GL_BLEND);

		/* Draw border. */
		glLineWidth(PANEL_BWIDTH);
		glBegin(GL_LINE_LOOP);
		glColor4f(0.2, 0.4, 0.6, 1.0);
		glVertex2d(p->p_u,		p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v);
		glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
		glVertex2d(p->p_u,		p->p_v - p->p_h);
		glEnd();
	}

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

__inline void
make_panel(struct panel *p, int wid, int compile, int toff)
{
	if (compile) {
		if (p->p_dl[wid])
			glDeleteLists(p->p_dl[wid], 1);
		p->p_dl[wid] = glGenLists(1);
		glNewList(p->p_dl[wid], GL_COMPILE);
	}

	draw_panel(p, toff);

	if (compile) {
		glEndList();
		p->p_opts &= ~POPT_DIRTY;
		p->p_opts |= POPT_COMPILE;
		glCallList(p->p_dl[wid]);
	}
}

void
panel_draw(struct panel *p, int wid)
{
	int compile, tweenadj, u, v, w, h;
	int lineno, curlen;
	int toff;
	char *s;

	toff = PANEL_PADDING + PANEL_BWIDTH;

	/*
	 * The second window should see the same panel
	 * as the right, so skip calculation (it would
	 * actually be very bad).
	 */
	if (stereo_mode == STM_PASV && wid == WINID_LEFT) {
		/*
		 * make_panel() on the first window
		 * will have cleared the dirty bit.
		 */
		make_panel(p, wid, p->p_opts & POPT_COMPILE, toff);
		p->p_opts &= ~POPT_COMPILE;
		return;
	}

	if ((p->p_opts & POPT_DIRTY) == 0 && p->p_dl[wid]) {
		glCallList(p->p_dl[wid]);
		goto done;
	}

	if (p->p_opts & POPT_REMOVE) {
		w = 0;
		h = 0;
		switch (p->p_stick) {
		case PSTICK_TL:
			u = -p->p_w;
			v = win_height - panel_offset[PVOFF_TL];
			break;
		case PSTICK_BL:
			u = -p->p_w;
			v = panel_offset[PVOFF_BL];
			break;
		case PSTICK_TR:
			u = win_width;
			v = win_height - panel_offset[PVOFF_TR];
			break;
		case PSTICK_BR:
			u = win_width;
			v = panel_offset[PVOFF_BR];
			break;
		default:
			u = p->p_u;
			v = p->p_v;
			break;
		}
	} else {
		lineno = 1;
		curlen = w = 0;
		for (s = p->p_str; *s != '\0'; s++, curlen++) {
			if (*s == '\n') {
				if (curlen > w)
					w = curlen;
				curlen = -1;
				lineno++;
			}
		}
		if (curlen > w)
			w = curlen;
		w = w * LETTER_WIDTH + 2 * toff;
		h = lineno * LETTER_HEIGHT + 2 * toff;
		if (p->p_nwidgets) {
			w = MAX(w, 2 * (LETTER_WIDTH * (int)p->p_maxwlen +
			    PWIDGET_LENGTH + 2 * PWIDGET_PADDING) + PWIDGET_PADDING);
			w = MIN(w, 2 * PWLABEL_LENGTH);
			/* p_nwidgets + 1 for truncation */
			h += ((p->p_nwidgets + 1) / 2) * PWIDGET_HEIGHT +
			    ((p->p_nwidgets + 1) / 2 - 1) * PWIDGET_PADDING;
		}
		switch (p->p_stick) {
		case PSTICK_TL:
			u = 0;
			v = win_height - panel_offset[PVOFF_TL];
			break;
		case PSTICK_BL:
			u = 0;
			v = panel_offset[PVOFF_BL] + p->p_h;
			break;
		case PSTICK_TR:
			u = win_width - w;
			v = win_height - panel_offset[PVOFF_TR];
			break;
		case PSTICK_BR:
			u = win_width - w;
			v = panel_offset[PVOFF_BR] + p->p_h;
			break;
		default:
			u = p->p_u;
			v = p->p_v;
			break;
		}
	}
	if (p->p_opts & POPT_MOBILE) {
		u = p->p_u;
		v = p->p_v;
	}

	tweenadj = MAX(abs(p->p_u - u), abs(p->p_v - v));
	tweenadj = MAX(tweenadj, abs(p->p_w - w));
	tweenadj = MAX(tweenadj, abs(p->p_h - h));
	if (tweenadj) {
		tweenadj = sqrt(tweenadj);
		if (!tweenadj)
			tweenadj = 1;
		/*
		 * The panel is being animated, so don't waste
		 * time recompiling it.
		 */
		compile = 0;
	} else
		compile = 1;

	if (p->p_u != u)
		p->p_u -= (p->p_u - u) / tweenadj;
	if (p->p_v != v)
		p->p_v -= (p->p_v - v) / tweenadj;
	if (p->p_w != w)
		p->p_w -= (p->p_w - w) / tweenadj;
	if (p->p_h != h)
		p->p_h -= (p->p_h - h) / tweenadj;

	if (p->p_opts & POPT_REMOVE) {
		if ((p->p_stick == PSTICK_TL && p->p_u + p->p_w <= 0) ||
		    (p->p_stick == PSTICK_BL && p->p_u + p->p_w <= 0) ||
		    (p->p_stick == PSTICK_TR && p->p_u >= win_width) ||
		    (p->p_stick == PSTICK_BR && p->p_u >= win_width)) {
			panel_free(p);
			return;
		}
	}

	make_panel(p, wid, compile, toff);

done:
	/* spacing */
	switch (p->p_stick) {
	case PSTICK_TL:
		panel_offset[PVOFF_TL] += p->p_h + 3;
		break;
	case PSTICK_TR:
		panel_offset[PVOFF_TR] += p->p_h + 3;
		break;
	case PSTICK_BL:
		panel_offset[PVOFF_BL] += p->p_h + 3;
		break;
	case PSTICK_BR:
		panel_offset[PVOFF_BR] += p->p_h + 3;
		break;
	}
}

void
panel_set_content(struct panel *p, char *fmt, ...)
{
	struct panel *t;
	va_list ap;
	size_t len;

	len = 0; /* gcc */
	p->p_opts |= POPT_DIRTY;
	/* XXX: use asprintf */
	for (;;) {
		va_start(ap, fmt);
		len = vsnprintf(p->p_str, p->p_strlen,
		    fmt, ap);
		va_end(ap);
		if (len >= p->p_strlen) {
			p->p_strlen = len + 1;
			if ((p->p_str = realloc(p->p_str,
			    p->p_strlen)) == NULL)
				err(1, "realloc");
		} else
			break;
	}
	if (p->p_str[len - 1] == '\n')
		p->p_str[len - 1] = '\0';
	/* All panels below us must be refreshed now, too. */
	for (t = p; t != TAILQ_END(&panels); t = TAILQ_NEXT(t, p_link))
		t->p_opts |= POPT_DIRTY;
}

void
panel_add_content(struct panel *p, char *fmt, ...)
{
	char *oldstr, *newstr;
	va_list ap;

	if ((oldstr = strdup(p->p_str)) == NULL)
		err(1, "strdup");

	va_start(ap, fmt);
	if (vasprintf(&newstr, fmt, ap) == -1)
		err(1, "vasprintf");
	va_end(ap);

	panel_set_content(p, "%s%s", oldstr, newstr);

	free(newstr);
	free(oldstr);
}

__inline int
panel_ready(struct panel *p)
{
	return ((p->p_opts & POPT_DIRTY) == 0 && p->p_str != NULL);
}

struct pwidget *
panel_get_pwidget(struct panel *p, struct pwidget *pw, struct pwidget **nextp)
{
	if (pw == NULL) {
		if ((pw = malloc(sizeof(*pw))) == NULL)
			err(1, "malloc");
		SLIST_INSERT_HEAD(&p->p_widgets, pw, pw_next);
//		memset(pw, 0, sizeof(pw));
		*nextp = NULL;
	} else
		*nextp = SLIST_NEXT(pw, pw_next);
	p->p_nwidgets++;
	return (pw);
}

void
pwidget_set(struct panel *p, struct pwidget *pw, struct fill *fp, char *s)
{
	pw->pw_fillp = fp;
	pw->pw_str = s;
	p->p_maxwlen = MAX(p->p_maxwlen, strlen(s));
}

void
panel_refresh_fps(struct panel *p)
{
	static long ofps = -1;

	if (ofps == fps && panel_ready(p))
		return;
	ofps = fps;
	panel_set_content(p, "FPS: %d", fps);
}

void
panel_refresh_cmd(struct panel *p)
{
	/* XXX:  is the mode_data_clean check correct? */
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 &&
	    selnode_clean & PANEL_CMD && (SLIST_EMPTY(&selnodes) ||
	    (mode_data_clean & PANEL_CMD) == 0) && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;
	mode_data_clean |= PANEL_CMD;
	selnode_clean |= PANEL_CMD;

	if (SLIST_EMPTY(&selnodes))
		panel_set_content(p,
		    "Send Command To Node\nNo node(s) selected.");
	else
		panel_set_content(p,
		    "Send Command To Node\n\nnid%d> %s",
		    SLIST_FIRST(&selnodes)->sn_nodep->n_nid,
		    buf_get(&uinp.uinp_buf));
}

void
panel_refresh_legend(struct panel *p)
{
	struct pwidget *pw, *nextp;
	size_t j;

	if ((mode_data_clean & PANEL_LEGEND) && panel_ready(p))
		return;
	mode_data_clean |= PANEL_LEGEND;

	p->p_nwidgets = 0;
	p->p_maxwlen = 0;
	pw = SLIST_FIRST(&p->p_widgets);
	switch (st.st_mode) {
	case SM_JOB:
		panel_set_content(p, "Job Legend\nTotal jobs: %lu",
		    job_list.ol_cur);
		for (j = 0; j < NSC; j++, pw = nextp) {
			if (j == SC_USED)
				continue;
			pw = panel_get_pwidget(p, pw, &nextp);
			pwidget_set(p, pw, &statusclass[j].nc_fill,
			    statusclass[j].nc_name);
		}
		for (j = 0; j < job_list.ol_cur; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pwidget_set(p, pw, &job_list.ol_jobs[j]->j_fill,
			    job_list.ol_jobs[j]->j_jname);
		}
		break;
	case SM_FAIL:
		panel_set_content(p, "Failure Legend\nTotal: %lu",
		    total_failures);

		pw = panel_get_pwidget(p, pw, &nextp);
		pwidget_set(p, pw, &fill_nodata, "No data");

		pw = nextp;
		for (j = 0; j < FAIL_NFAILS; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pwidget_set(p, pw, &failclass[j].nc_fill,
			    failclass[j].nc_name);
		}
		break;
	case SM_TEMP:
		panel_set_content(p, "Temperature Legend");

		pw = panel_get_pwidget(p, pw, &nextp);
		pwidget_set(p, pw, &fill_nodata, "No data");

		pw = nextp;
		for (j = 0; j < TEMP_NTEMPS; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pwidget_set(p, pw, &tempclass[j].nc_fill,
			    tempclass[j].nc_name);
		}
		break;
	case SM_YOD:
		panel_set_content(p, "Yod Legend\nTotal yods: %lu",
		    yod_list.ol_cur);
		for (j = 0; j < NSC; j++, pw = nextp) {
			if (j == SC_USED)
				continue;
			pw = panel_get_pwidget(p, pw, &nextp);
			pwidget_set(p, pw, &statusclass[j].nc_fill,
			    statusclass[j].nc_name);
		}
		for (j = 0; j < yod_list.ol_cur; j++, pw = nextp) {
			pw = panel_get_pwidget(p, pw, &nextp);
			pwidget_set(p, pw, &yod_list.ol_yods[j]->y_fill,
			    yod_list.ol_yods[j]->y_cmd);
		}
		break;
	}
}

void
panel_refresh_ninfo(struct panel *p)
{
	struct physcoord pc;
	struct objhdr *ohp;
	struct objlist *ol;
	struct selnode *sn;
	struct ivec *iv;
	struct node *n;

	if (selnode_clean & PANEL_NINFO && (SLIST_EMPTY(&selnodes) ||
	    (mode_data_clean & PANEL_NINFO)) && panel_ready(p))
		return;
	mode_data_clean |= PANEL_NINFO;
	selnode_clean |= PANEL_NINFO;

	if (SLIST_EMPTY(&selnodes)) {
		panel_set_content(p, "Node Information\nNo node(s) selected");
		return;
	}

	if (nselnodes > 1) {
		char *label, nids[BUFSIZ], data[BUFSIZ];
		size_t j, nids_pos, data_pos;

		j = 0;
		nids[0] = data[0] = '\0';
		nids_pos = data_pos = 0;
		SLIST_FOREACH(sn, &selnodes, sn_next) {
			n = sn->sn_nodep;

			nids_pos += snprintf(nids + nids_pos,
			    sizeof(nids) - nids_pos, ",%d", n->n_nid);

			if (nids_pos >= sizeof(nids))
				break;
			switch (st.st_mode) {
			case SM_JOB:
				if (n->n_state == SC_USED)
					n->n_job->j_oh.oh_flags |= OHF_TMP;
				break;
			case SM_YOD:
				if (n->n_yod)
					n->n_yod->y_oh.oh_flags |= OHF_TMP;
				break;
			}
			j++;
		}
		text_wrap(nids, sizeof(nids), 50);

		label = NULL; /* gcc */
		ol = NULL; /* gcc */
		switch (st.st_mode) {
		case SM_JOB:
			label = "Job ID(s)";
			ol = &job_list;
			break;
		case SM_TEMP:
			label = "Temperature(s)";
			break;
		case SM_FAIL:
			label = "Failure(s)";
			break;
		case SM_YOD:
			label = "Yod ID(s)";
			ol = &yod_list;
			break;
		}

		for (j = 0; ol && j < ol->ol_cur; j++) {
			ohp = ol->ol_data[j];
			if (ohp->oh_flags & OHF_TMP) {
				ohp->oh_flags &= ~OHF_TMP;
				switch (st.st_mode) {
				case SM_JOB:
					data_pos += snprintf(data + data_pos,
					    sizeof(data) - data_pos, ",%d",
					    ((struct job *)ohp)->j_id);
					break;
				case SM_YOD:
					data_pos += snprintf(data + data_pos,
					    sizeof(data) - data_pos, ",%d",
					    ((struct yod *)ohp)->y_id);
					break;
				}
			}
			if (data_pos >= sizeof(data))
				break;
		}
		text_wrap(data, sizeof(data), 50);

		if (data[0] == '\0')
			strncpy(data, "_(none)", sizeof(data) - 1);
		data[sizeof(data) - 1] = '\0';

		panel_set_content(p,
		    "Node Information\n"
		    "%d node(s) selected\n"
		    "Node ID(s): %s\n"
		    "%s: %s",
		    nselnodes, nids + 1,
		    label, data + 1);
		return;
	}

	/* Only one selected node. */
	n = SLIST_FIRST(&selnodes)->sn_nodep;
	node_physpos(n, &pc);
	iv = &n->n_wiv;

	panel_set_content(p,
	    "Node Information\n"
	    "Node ID: %d\n"
	    "Wired position: (%d,%d,%d)\n"
	    "Physical position: (%d,%d,%d,%d,%d)\n"
	    "Temperature: %d\n"
	    "# Failures: %d",
	    n->n_nid,
	    iv->iv_x, iv->iv_y, iv->iv_z,
	    pc.pc_r, pc.pc_cb, pc.pc_cg, pc.pc_m, pc.pc_n,
	    n->n_temp, n->n_fails);
	if (n->n_job)
		panel_add_content(p,
		    "\n"
		    "Job ID: %d\n"
		    "Job owner: %s\n"
		    "Job name: %s\n"
		    "Job queue: %s\n"
		    "Job duration: %d:%02d\n"
		    "Job time used: %d:%02d (%d%%)\n"
		    "Job CPUs: %d",
		    n->n_job->j_id,
		    n->n_job->j_owner,
		    n->n_job->j_jname,
		    n->n_job->j_queue,
		    n->n_job->j_tmdur / 60,
		    n->n_job->j_tmdur % 60,
		    n->n_job->j_tmuse / 60,
		    n->n_job->j_tmuse % 60,
		    n->n_job->j_tmuse * 100 /
		      (n->n_job->j_tmdur ?
		       n->n_job->j_tmdur : 1),
		    n->n_job->j_ncpus);
	else
		panel_add_content(p,
		    "\n"
		    "Type: %s",
		    statusclass[n->n_state].nc_name);
	if (n->n_yod) {
		char cmdbuf[YFL_CMD];

		strncpy(cmdbuf, n->n_yod->y_cmd, sizeof(cmdbuf) - 1);
		cmdbuf[sizeof(cmdbuf) - 1] = '\0';
		text_wrap(cmdbuf, sizeof(cmdbuf), 50);

		panel_add_content(p,
		    "\n"
		    "Yod ID: %d\n"
		    "Yod partition ID: %d\n"
		    "Yod CPUs: %d\n"
		    "Yod command: %s",
		    n->n_yod->y_id,
		    n->n_yod->y_partid,
		    n->n_yod->y_ncpus,
		    cmdbuf);
	}
}

void
panel_refresh_flyby(struct panel *p)
{
	static int sav_mode = -1;

	if (sav_mode == flyby_mode && panel_ready(p))
		return;
	sav_mode = flyby_mode;

	switch (flyby_mode) {
	case FBM_PLAY:
		panel_set_content(p, "Playing flyby");
		break;
	case FBM_REC:
		panel_set_content(p, "Recording flyby");
		break;
	default:
		panel_set_content(p, "Flyby mode disabled");
		break;
	}
}

void
panel_refresh_goto(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "Go To Node\nNode ID: %s",
	    buf_get(&uinp.uinp_buf));
}

void
panel_refresh_mem(struct panel *p)
{
	if ((mode_data_clean & PANEL_MEM) && panel_ready(p))
		return;
	mode_data_clean |= PANEL_MEM;
	panel_set_content(p, "Memory Usage\nVSZ: %lu\nRSS: %ld\n", vmem, rmem);
}

void
panel_refresh_pos(struct panel *p)
{
	if (!cam_dirty && panel_ready(p))
		return;

	panel_set_content(p, "Position (%.2f,%.2f,%.2f)\n"
	    "Look (%.2f,%.2f,%.2f)\n"
	    "Roll (%.2f,%.2f,%.2f)",
	    st.st_x, st.st_y, st.st_z,
	    st.st_lx, st.st_ly, st.st_lz,
	    st.st_ux, st.st_uy, st.st_uz);
}

void
panel_refresh_ss(struct panel *p)
{
	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p))
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, "Screenshot filename: %s",
	    buf_get(&uinp.uinp_buf));
}

#define BLINK_INTERVAL 1000000
void
panel_refresh_eggs(struct panel *p)
{
	char *s[] = {"Follow the white rabbit...\n  %s",
			"Follow the white rabbit...\n> %s"};
	static struct timeval pre = {0,0};
	static int i = 0;
	int dirty;

	dirty = panel_blink(&pre, s, 2, &i, BLINK_INTERVAL);

	if ((uinp.uinp_opts & UINPO_DIRTY) == 0 && panel_ready(p) && !dirty)
		return;
	uinp.uinp_opts &= ~UINPO_DIRTY;

	panel_set_content(p, s[i], buf_get(&uinp.uinp_buf));
}

void
panel_date_invalidate(__unused int a)
{
	struct panel *p;

	p = panel_for_id(PANEL_DATE);
	if (p == NULL)
		return;
//		errx(1, "internal error: date invalidate callback "
//		    "called but no date panel present");
	p->p_opts |= POPT_USRDIRTY;
}

#define TMBUF_SIZ 30

void
panel_refresh_date(struct panel *p)
{
	static char tmbuf[TMBUF_SIZ];
	struct tm tm;
	time_t now;

	if ((p->p_opts & POPT_USRDIRTY) == 0 &&
	    panel_ready(p))
		return;
	if (p->p_str == NULL ||
	    p->p_opts & POPT_USRDIRTY) {
		p->p_opts &= ~POPT_USRDIRTY;

		/*
		 * If a session is live, (other than race
		 * conditions) the directory should exit.
		 */
		if (ssp != NULL) {
			char fn[PATH_MAX];
			struct stat stb;

			snprintf(fn, sizeof(fn), "%s/%s",
			    _PATH_SESSIONS, ssp->ss_sid);
			if (stat(fn, &stb) == -1)
				err(1, "stat %s", fn);
			now = stb.st_mtime;
		} else {
			time(&now);
			glutTimerFunc(1000 * (60 - tm.tm_sec),
			    panel_date_invalidate, 0);
		}
		localtime_r(&now, &tm);
		strftime(tmbuf, sizeof(tmbuf), "%b %e %y %H:%M", &tm);
		panel_set_content(p, "(c) 2005 PSC\n%s", tmbuf);
	}
}

void
panel_refresh_status(struct panel *p)
{
	if (panel_ready(p))
		return;
	panel_set_content(p, "Status\n%s", status_get());
}

void
draw_panels(int wid)
{
	struct panel *p, *np;

	/*
	 * Can't use TAILQ_FOREACH() since draw_panel() may remove
	 * a panel.
	 */
	memset(panel_offset, 0, sizeof(panel_offset));
	for (p = TAILQ_FIRST(&panels); p != TAILQ_END(&panels); p = np) {
		np = TAILQ_NEXT(p, p_link);
		if (stereo_mode != STM_PASV ||
		   wid == WINID_RIGHT)
			p->p_refresh(p);
		panel_draw(p, wid);
	}
}

struct panel *
panel_for_id(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id)
			return (p);
	return (NULL);
}

void
panel_show(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id) {
			if (p->p_opts & POPT_REMOVE)
				panel_tremove(p);
			return;
		}
	panel_toggle(id);
}

void
panel_hide(int id)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == id) {
			if ((p->p_opts & POPT_REMOVE) == 0)
				panel_tremove(p);
			return;
		}
}

void
panel_tremove(struct panel *p)
{
	struct panel *t;

	/* XXX inspect POPT_REMOVE first? */
	p->p_opts ^= POPT_REMOVE;
	for (t = p; t != TAILQ_END(&panels); t = TAILQ_NEXT(t, p_link))
		t->p_opts |= POPT_DIRTY;
	if (flyby_mode == FBM_REC)
		flyby_writepanel(p->p_id);
}

void
panel_toggle(int panel)
{
	struct pinfo *pi;
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link)
		if (p->p_id == panel) {
			panel_tremove(p);
			return;
		}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	memset(p, 0, sizeof(*p));

	pi = &pinfo[baseconv(panel) - 1];

	p->p_str = NULL;
	p->p_refresh = pi->pi_refresh;
	p->p_id = panel;
	p->p_stick = PSTICK_TR;
	p->p_u = win_width;
	p->p_v = win_height - panel_offset[PVOFF_TR] - 1;
	p->p_fill.f_r = 1.0f;
	p->p_fill.f_g = 1.0f;
	p->p_fill.f_b = 1.0f;
	p->p_fill.f_a = 1.0f;
	SLIST_INIT(&p->p_widgets);

	if (pi->pi_opts & PF_UINP) {
		glutKeyboardFunc(gl_keyh_uinput);
		uinp.uinp_panel = p;
		uinp.uinp_opts = pi->pi_uinpopts;
		uinp.uinp_callback = pi->pi_uinpcb;
	}
	TAILQ_INSERT_TAIL(&panels, p, p_link);
	if (flyby_mode == FBM_REC)
		flyby_writepanel(p->p_id);
}

void
panel_demobilize(struct panel *p)
{
	p->p_opts &= ~POPT_MOBILE;
	p->p_opts |= POPT_DIRTY;
	if (p->p_u + p->p_w / 2 < win_width / 2) {
		if (win_height - p->p_v + p->p_h / 2 < win_height / 2)
			p->p_stick = PSTICK_TL;
		else
			p->p_stick = PSTICK_BL;
	} else {
		if (win_height - p->p_v + p->p_h / 2 < win_height / 2)
			p->p_stick = PSTICK_TR;
		else
			p->p_stick = PSTICK_BR;
	}
}

void
flip_panels(int panels)
{
	int b;

	while (panels) {
		b = ffs(panels) - 1;
		panels &= ~(1 << b);
		panel_toggle(1 << b);
	}
}

/*
 * Set panels state to those specified in `start.'
 */
void
init_panels(int start)
{
	struct panel *p;
	int cur;

	cur = 0;
	TAILQ_FOREACH(p, &panels, p_link)
		cur |= p->p_id;
	flip_panels((cur ^ start) & ~FB_PMASK);
}

/* Blink panel text (swap between two strings during interval). */
int panel_blink(struct timeval *pre, char **s, int size, int *i, long interval)
{
	struct timeval tv, diff;
	char *str = s[*i];
	int tog = 0;

	gettimeofday(&tv, NULL);
	timersub(&tv, pre, &diff);

	/* Make it blink once per second */
	if(diff.tv_sec * 1e6 + diff.tv_usec > interval) {
		*pre = tv;

		/* Swap */
		if(++(*i) == size)
			*i = 0;
		str = s[*i];
		tog = 1;
	}

	return tog;
}
