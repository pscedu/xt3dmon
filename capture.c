/* $Id$ */

#include "compat.h"

#include <err.h>
#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mon.h"

#define NUM_FRAMES	200
#define MAX_FRAMES	25000

void ppm_write(FILE *, unsigned char *, long, long);

struct capture_format {
	const char	 *cf_ext;
	int		  cf_size;
	void		(*cf_writef)(FILE *, unsigned char *, long, long);
	int		  cf_glmode;
} capture_formats[] = {
	/* These must correspond to the CM_* constants. */
	{ "png", 4, png_write, GL_RGBA },
	{ "ppm", 3, ppm_write, GL_RGB },
};

static unsigned char *fbuf[NUM_FRAMES];
static int stereo_left;
static int fbuf_pos;
static int capture_pos;

/* Save buffer as PPM. */
void
ppm_write(FILE *fp, unsigned char *buf, long w, long h)
{
	fprintf(fp, "P6 %ld %ld %d\n", w, h, 255);
	fwrite(buf, 3 * w * h, 1, fp);
}

void
capture_writeback(int mode)
{
	char fn[PATH_MAX];
	const char *ext;
	FILE *fp;
	int k;

	ext = capture_formats[mode].cf_ext;
	for (k = 0; k < fbuf_pos; k++, capture_pos++) {
		if (stereo_mode) {
			snprintf(fn, sizeof(fn), "%s/%c%07d.%s",
			    _PATH_SSDIR, stereo_left ? 'l' : 'r',
			    capture_pos / 2, ext);
			stereo_left = !stereo_left;
		} else
			snprintf(fn, sizeof(fn), "%s/%07d.%s",
			    _PATH_SSDIR, capture_pos, ext);
		if ((fp = fopen(fn, "wb")) == NULL)
			err(1, "%s", fn);
		capture_formats[mode].cf_writef(fp, fbuf[k],
		    win_width, win_height);
		fclose(fp);
	}
}

void
capture_begin(int mode)
{
	long size = 0;
	int i;

	glMatrixMode(GL_PROJECTION);
	glMatrixMode(GL_MODELVIEW);

	/*
	 * XXX: make sure this gets called whenever
	 * the output file format changes.
	 */
	size = win_width * win_height * capture_formats[mode].cf_size;

	for (i = 0; i < NUM_FRAMES; i++)
		if ((fbuf[i] = malloc(size * sizeof(*fbuf[i]))) == NULL)
			err(1, "malloc");
	stereo_left = 1;
}

void
capture_end(void)
{
	int i;

	capture_writeback(capture_mode);
	for (i = 0; i < NUM_FRAMES; i++)
		free(fbuf[i]);
}

void
capture_copyfb(int mode, unsigned char *buf)
{
	glMatrixMode(GL_PROJECTION);

	/* Read data from the framebuffer. */
	glReadPixels(0, 0, win_width, win_height,
	    capture_formats[mode].cf_glmode, GL_UNSIGNED_BYTE, buf);

	glMatrixMode(GL_MODELVIEW);
}

/* Capture a frame. */
void
capture_frame(int mode)
{
	/*
	 * After we reach max frames, dump them.  NOTE: this is
	 * nasty and will take awhile.
	 */
	if (fbuf_pos + 1 >= NUM_FRAMES) {
		capture_writeback(mode);
		fbuf_pos = 0;
	}
	capture_copyfb(mode, fbuf[fbuf_pos++]);
	if (stereo_mode) {
		glDrawBuffer(GL_BACK_RIGHT);
		capture_copyfb(mode, fbuf[fbuf_pos++]);
		glDrawBuffer(GL_BACK_LEFT);
	}
}

/* Take a screenshot. */
void
capture_snap(const char *fn, int mode)
{
	static unsigned char *buf;
	long size;
	FILE *fp;

	size = capture_formats[mode].cf_size *
	    win_width * win_height;
	if ((buf = realloc(buf, size * sizeof(*buf))) == NULL)
		err(1, "realloc");
	capture_copyfb(mode, buf);

	/* Write data to buffer. */
	if ((fp = fopen(fn, "wb")) == NULL)
		err(1, "%s", fn);
	capture_formats[mode].cf_writef(fp, buf, win_width, win_height);
	fclose(fp);
}

void
capture_snapfd(int fd, int mode)
{
	static unsigned char *buf;
	long size;
	FILE *fp;

	size = capture_formats[mode].cf_size *
	    win_width * win_height;
	if ((buf = realloc(buf, size * sizeof(*buf))) == NULL)
		err(1, "realloc");
	capture_copyfb(mode, buf);

	/* Write data to buffer. */
	if ((fp = fdopen(fd, "wb")) == NULL)
		err(1, "fd %d", fd);
	capture_formats[mode].cf_writef(fp, buf, win_width, win_height);
	fclose(fp);
}
