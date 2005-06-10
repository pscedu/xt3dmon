/* $Id$ */

#include <sys/types.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mon.h"
#include "queue.h"
#include "buf.h"

#define LETTER_HEIGHT 13
#define LETTER_WIDTH 8
#define PANEL_PADDING 3
#define PANEL_BWIDTH 2.0f

void panel_refresh_fps(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_cmd(struct panel *);
void panel_refresh_legend(struct panel *);
void panel_refresh_flyby(struct panel *);

void (*refreshf[])(struct panel *) = {
	panel_refresh_fps,
	panel_refresh_ninfo,
	panel_refresh_cmd,
	panel_refresh_legend,
	panel_refresh_flyby
};

struct panels panels;
int panel_offset;

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
draw_panel(struct panel *p)
{
	int lineno, curlen, line_offset, toff;
	int u, v, w, h;
	char *s;



	toff = PANEL_PADDING + PANEL_BWIDTH;

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
	if (p->p_removing) {
		u = win_width;
		v = win_height;
		w = 0;
		h = 0;
	} else {
		u = win_width - w;
		v = win_height - panel_offset;
		w = w * LETTER_WIDTH + 2 * toff;
		h = lineno * LETTER_HEIGHT + 2 * toff;
	}

	if (p->p_u != u)
		p->p_u += p->p_u - u < 0 ? 1 : -1;
	if (p->p_v != v)
		p->p_v += p->p_v - v < 0 ? 1 : -1;
	if (p->p_w != w)
		p->p_w += p->p_w - w < 0 ? 1 : -1;
	if (p->p_h != h)
		p->p_h += p->p_h - h < 0 ? 1 : -1;

	if (p->p_removing && p->p_u >= win_width) {
		TAILQ_REMOVE(&panels, p, p_link);
		free(p->p_str);
		free(p);
	}

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
	line_offset = p->p_v - toff - LETTER_HEIGHT;
	glRasterPos2d(p->p_u + toff, line_offset);
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n') {
			line_offset -= LETTER_HEIGHT;
			glRasterPos2d(p->p_u + toff, line_offset);
		} else
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
	}

	/* Draw background. */
	glBegin(GL_POLYGON);
	glColor4f(0.20, 0.40, 0.5, 1.0);
	glVertex2d(p->p_u,		p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
	glVertex2d(p->p_u,		p->p_v - p->p_h);
	glEnd();

	/* Draw border. */
	glLineWidth(PANEL_BWIDTH);
	glBegin(GL_LINE_LOOP);
	glColor4f(0.40, 0.80, 1.0, 1.0);
	glVertex2d(p->p_u,		p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v);
	glVertex2d(p->p_u + p->p_w,	p->p_v - p->p_h);
	glVertex2d(p->p_u,		p->p_v - p->p_h);
	glEnd();

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	panel_offset += p->p_h + 3; /* spacing */
}

void
panel_set_content(struct panel *p, char *fmt, ...)
{
	va_list ap;
	size_t len;

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
}

void
panel_refresh_fps(struct panel *p)
{
	panel_set_content(p, "FPS: %d", fps);
}

void
panel_refresh_cmd(struct panel *p)
{
	if (st.st_selnode)
		panel_set_content(p, "Sending command to host\n\n> %s", 
		    buf_get(&cmdbuf));
	else
		panel_set_content(p, "Please select a node\nto send a command to.");
}

void
panel_refresh_legend(struct panel *p)
{
	switch (st.st_mode) {
	case SM_JOBS:
		panel_set_content(p, "Jobs: %d", njobs);
		break;
	case SM_FAIL:
		panel_set_content(p, "Failures: %d", total_failures);
		break;
	case SM_TEMP:
		break;
	}
}

void
panel_refresh_ninfo(struct panel *p)
{
	if (st.st_selnode) {
		switch (st.st_selnode->n_savst) {
		case JST_USED:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Owner: %d\n"
			    "Job name: [%s]\n"
			    "Duration: %d\n"
			    "CPUs: %d",
			    st.st_selnode->n_nid,
			    st.st_selnode->n_job->j_owner,
			    st.st_selnode->n_job->j_name,
			    st.st_selnode->n_job->j_dur,
			    st.st_selnode->n_job->j_cpus);
			break;
		default:
			panel_set_content(p,
			    "Node ID: %d",
			    st.st_selnode->n_nid);
			break;
		}
	} else {
		panel_set_content(p, "Select a node");
	}
}

void
panel_refresh_flyby(struct panel *p)
{
	panel_set_content(p, "Flyby");
}

void
adjpanels(void)
{
	struct panel *p;
return;
	/* Resize happening -- move panels. */
	float v = win_height;
	TAILQ_FOREACH(p, &panels, p_link) {
//		if (p->p_su < win_width - p->p_w)
//			p->p_adju = 1;
//		else if (p->p_su > win_width - p->p_w)
//			p->p_adju = -1;
//		p->p_su = win_width - p->p_w;

//		if (p->p_sv < v)
//			p->p_adjv = 1;
//		else if (p->p_sv < v)
//			p->p_adjv = -1;
//		p->p_sv = v;

//		v -= p->p_h;
	}
}

void
draw_panels(void)
{
	struct panel *p, *np;

	/*
	 * Can't use TAILQ_FOREACH() since draw_panel() may remove
	 * a panel.
	 */
	panel_offset = 1;
	for (p = TAILQ_FIRST(&panels); p != TAILQ_END(&panels); p = np) {
		np = TAILQ_NEXT(p, p_link);
		p->p_refresh(p);
		draw_panel(p);
	}
}

void
panel_toggle(int panel)
{
	struct panel *p;

	TAILQ_FOREACH(p, &panels, p_link) {
		if (p->p_id == panel) {
			/* Found; toggle existence. */
			p->p_removing = !p->p_removing;
printf("queueing\n");
			return;
		}
	}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	memset(p, 0, sizeof(*p));
	p->p_str = NULL;
	p->p_strlen = 0;
	p->p_refresh = refreshf[baseconv(panel) - 1];
	p->p_id = panel;
	p->p_su = win_width;
	p->p_sv = win_height - panel_offset - 1;
	p->p_fill.f_r = 1.0f;
	p->p_fill.f_g = 1.0f;
	p->p_fill.f_b = 1.0f;
	p->p_fill.f_a = 1.0f;
	TAILQ_INSERT_TAIL(&panels, p, p_link);
}
