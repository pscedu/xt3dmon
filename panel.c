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

#define LETTER_HEIGHT 13
#define LETTER_WIDTH 8

void panel_refresh_fps(struct panel *);
void panel_refresh_ninfo(struct panel *);
void panel_refresh_cmd(struct panel *);
void panel_refresh_flegend(struct panel *);
void panel_refresh_jlegend(struct panel *);

void (*refreshf[])(struct panel *) = {
	panel_refresh_fps,
	panel_refresh_ninfo,
	panel_refresh_cmd,
	panel_refresh_flegend,
	panel_refresh_jlegend
};

struct panel_slh panels;

int
baseconv(int n)
{
	unsigned int i;
	
	for (i = 0; i < sizeof(n) * 8; i++)
		if (n & (1 << i))
			return (i + 1);
	return (0);
}

/* Return the height of the panel.  I know, hackish. */
float
draw_panel(struct panel *p, float offset)
{
	int lineno, curlen, off;
	char *s;

#if 0
	if (p->p_adju) {
		p->p_su += p->p_adju;
		if (p->p_su <= 0) {
printf("removing\n");
			SLIST_REMOVE(&panels, p, panel, p_next);
			free(p->p_str);
			free(p);
			return (0.0f);
		} else if (p->p_su >= p->p_u)
{printf("moved into place\n");
			p->p_adju = 0;
}
	}
	if (p->p_adjv) {
		p->p_sv += p->p_adjv;
//		if (p->p_sv >= )
	}
#endif

	/* Save state and set things up for 2D. */
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, win_width, 0.0, win_height);

	glColor4f(p->p_r, p->p_g, p->p_b, p->p_a);

	lineno = 1;
	curlen = p->p_w = 0;
	for (s = p->p_str; *s != '\0'; s++, curlen++) {
		if (*s == '\n') {
			if (curlen > p->p_w)
				p->p_w = curlen;
			curlen = 0;
			lineno++;
		}
	}
	if (curlen > p->p_w)
		p->p_w = curlen;
//printf("offset: %d\n", offset);
	p->p_w *= LETTER_WIDTH;
	p->p_h = lineno * LETTER_HEIGHT + offset;
	p->p_u = win_width - p->p_w;
	p->p_v = win_height - offset;

	if (p->p_su != p->p_u)
		p->p_su += p->p_su - p->p_u < 0 ? 1 : -1;
	if (p->p_sv != p->p_v)
		p->p_sv += p->p_sv - p->p_v < 0 ? 1 : -1;

//printf("write '%s'\n", p->p_str);
	/* Panel content. */
	off = 0;
	glRasterPos2d(p->p_su, p->p_sv - p->p_h);
	for (s = p->p_str; *s != '\0'; s++) {
		if (*s == '\n') {
			off += LETTER_HEIGHT;
			glRasterPos2d(p->p_su, p->p_sv - p->p_h - off);
		} else
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
	}

	/* Draw background. */
	glBegin(GL_POLYGON);
	glColor4f(0.20, 0.40, 0.5, 1.0);
	glVertex2d(p->p_su, p->p_sv);
	glVertex2d(p->p_su + p->p_w, p->p_sv);
	glVertex2d(p->p_su + p->p_w, p->p_sv - p->p_h);
	glVertex2d(p->p_su, p->p_sv - p->p_h);
	glEnd();

	/* Draw border. */
	glLineWidth(2.0);
	glBegin(GL_LINE_LOOP);
	glColor4f(0.40, 0.80, 1.0, 1.0);
	glVertex2d(p->p_su, p->p_sv);
	glVertex2d(p->p_su + p->p_w, p->p_sv);
	glVertex2d(p->p_su + p->p_w, p->p_sv - p->p_h);
	glVertex2d(p->p_su, p->p_sv - p->p_h);
	glEnd();

	/* End 2D mode. */
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	return (p->p_h - offset);
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
printf("resizing %d to %d\n", p->p_strlen, len);
			p->p_strlen = len + 1;
			if ((p->p_str = realloc(p->p_str, len)) == NULL)
				err(1, "realloc");
		} else
			break;
	}
}

void
panel_refresh_fps(struct panel *p)
{
//printf("refresh_fps()\n");
	panel_set_content(p, "FPS: %d", fps);
}

void
panel_refresh_cmd(struct panel *p)
{
	if (selnode)
		panel_set_content(p, "Sending command to host\n\n> ");
	else
		panel_set_content(p, "Please select a node\nto send a command to.");
}

void
panel_refresh_jlegend(struct panel *p)
{
	panel_set_content(p, "jlegend");
}

void
panel_refresh_flegend(struct panel *p)
{
	panel_set_content(p, "flegend.");
}

void
panel_refresh_ninfo(struct panel *p)
{
	if (selnode) {
		switch (selnode->n_state) {
		case ST_USED:
			panel_set_content(p,
			    "Node ID: %d\n"
			    "Owner: %d\n"
			    "Job name: %s\n"
			    "Duration: %d\n"
			    "CPUs: %d",
			    selnode->n_nid,
			    selnode->n_job->j_owner,
			    selnode->n_job->j_name,
			    selnode->n_job->j_dur,
			    selnode->n_job->j_cpus);
			break;
		default:
			panel_set_content(p,
			    "Node ID: %d\n",
			    selnode->n_nid);
			break;
		}
	} else {
		panel_set_content(p, "Select a node");
	}
}

void
adjpanels(void)
{
	struct panel *p;

	/* Resize happening -- move panels. */
	float v = win_height;
	SLIST_FOREACH(p, &panels, p_next) {
		if (p->p_su < win_width - p->p_w)
			p->p_adju = 1;
		else if (p->p_su > win_width - p->p_w)
			p->p_adju = -1;
		p->p_su = win_width - p->p_w;

		if (p->p_sv < v)
			p->p_adjv = 1;
		else if (p->p_sv < v)
			p->p_adjv = -1;
		p->p_sv = v;

		v -= p->p_h;
	}
}

void
draw_panels(void)
{
	struct panel *p, *np;
	float offset;

	offset = 0;
	/*
	 * Can't use SLIST_FOREACH() since draw_panel() may remove
	 * a panel.
	 */
	for (p = SLIST_FIRST(&panels); p != SLIST_END(&panels); p = np) {
		np = SLIST_NEXT(p, p_next);
		p->p_refresh(p);
		offset += draw_panel(p, offset);
	}
}

void
panel_toggle(int panel)
{
	struct panel *p;

	SLIST_FOREACH(p, &panels, p_next) {
		if (p->p_id == panel) {
			/* Found; make it disappear. */
			p->p_u = win_width;
			return;
		}
	}
	/* Not found; add. */
	if ((p = malloc(sizeof(*p))) == NULL)
		err(1, "malloc");
	SLIST_INSERT_HEAD(&panels, p, p_next);
	p->p_adju = 1;
	p->p_str = NULL;
	p->p_strlen = 0;
	p->p_refresh = refreshf[baseconv(panel)];
	p->p_id = panel;
	p->p_su = win_width;
	p->p_sv = win_height;
	p->p_r = 1.0f;
	p->p_g = 1.0f;
	p->p_b = 1.0f;
	p->p_a = 1.0f;
}
