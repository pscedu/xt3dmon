/* $Id$ */

#include "mon.h"

#include <err.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "cam.h"
#include "capture.h"
#include "draw.h"
#include "env.h"
#include "pathnames.h"
#include "state.h"
#include "status.h"
#include "xmath.h"

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

struct ivec virtwinv = { { 7200, 5400, 0 } };

int capture_mode = CM_PNG;
int capture_usevirtual;
int capture_pos;

/* Save buffer as PPM. */
void
ppm_write(FILE *fp, unsigned char *buf, long w, long h)
{
	size_t len;

	len = 3 * w * h;
	fprintf(fp, "P6 %ld %ld %d\n", w, h, 255);
	if (fwrite(buf, 1, len, fp) != len)
		err(1, "fwrite");
}

/* Copy frame buffer to another buffer. */
void
capture_copyfb(int mode, unsigned char *buf)
{
	glMatrixMode(GL_PROJECTION);

	/* Read data from the framebuffer. */
	glReadPixels(0, 0, winv.iv_w, winv.iv_h,
	    capture_formats[mode].cf_glmode, GL_UNSIGNED_BYTE, buf);

	glMatrixMode(GL_MODELVIEW);
}

const char *
capture_seqname(int mode)
{
	static char fn[PATH_MAX];
	const char *ext;

	capture_pos++;
	ext = capture_formats[mode].cf_ext;
	if (stereo_mode) {
		snprintf(fn, sizeof(fn), "%s/%c%07d.%s",
		    _PATH_SSDIR, wid == WINID_LEFT ? 'l' : 'r',
		    capture_pos / 2, ext);
	} else
		snprintf(fn, sizeof(fn), "%s/%07d.%s",
		    _PATH_SSDIR, capture_pos, ext);
	return (fn);
}

/* Take a screenshot. */
void
capture_snap(const char *fn, int mode)
{
	static unsigned char *buf;
	long size;
	FILE *fp;

	size = capture_formats[mode].cf_size *
	    winv.iv_w * winv.iv_h;
	if ((buf = realloc(buf, size * sizeof(*buf))) == NULL)
		err(1, "realloc");
	capture_copyfb(mode, buf);

	/* Write data to buffer. */
	/* XXX append file extension if not specified? */
	if ((fp = fopen(fn, "wb")) == NULL)
		err(1, "%s", fn);
	capture_formats[mode].cf_writef(fp, buf, winv.iv_w, winv.iv_h);
	fclose(fp);
}

void
capture_snapfd(int fd, int mode)
{
	static unsigned char *buf;
	long size;
	FILE *fp;
	int nfd;

	size = capture_formats[mode].cf_size *
	    winv.iv_w * winv.iv_h;
	if ((buf = realloc(buf, size * sizeof(*buf))) == NULL)
		err(1, "realloc");
	capture_copyfb(mode, buf);

	if ((nfd = dup(fd)) == -1)
		err(1, "dup");

	/* Write data to buffer. */
	if ((fp = fdopen(nfd, "wb")) == NULL)
		err(1, "fd %d", fd);
	capture_formats[mode].cf_writef(fp, buf, winv.iv_w, winv.iv_h);
	fclose(fp);
}

void
capture_virtual(const char *fn)
{
	int mode, rf, x, y, xdraws, ydraws, h, pos, fact;
	unsigned char *buf;
	size_t size;
	FILE *fp;

	if (virtwinv.iv_w % winv.iv_w ||
	    virtwinv.iv_h % winv.iv_h) {
		status_add(SLP_URGENT,
		    "virtual size not a factor of window size");
		return;
	}

	if (virtwinv.iv_w <= winv.iv_w ||
	    virtwinv.iv_h <= winv.iv_h) {
		status_add(SLP_URGENT,
		    "window size greater than virtual size");
		return;
	}

	mode = CM_PNG;
	fact = capture_formats[mode].cf_size;
	size = fact * virtwinv.iv_w * virtwinv.iv_h;
	if ((buf = calloc(size, sizeof(*buf))) == NULL)
		err(1, "calloc");

	xdraws = virtwinv.iv_w / winv.iv_w;
	ydraws = virtwinv.iv_w / winv.iv_w;

	for (x = 0; x < xdraws; x++)
		for (y = 0; y < ydraws; y++) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			frustum_virtual(x, xdraws, y, ydraws);
			glMatrixMode(GL_MODELVIEW);
			cam_look();

			rf = st.st_rf;
			draw_scene();
			if (st.st_rf != rf)
				errx(1, "internal error: draw_scene() modified "
				    "rebuild flags (%d<=>%d)", st.st_rf, rf);

			glMatrixMode(GL_PROJECTION);

			pos = fact * (x * winv.iv_w + y * winv.iv_h * virtwinv.iv_w);
			for (h = 0; h < winv.iv_h; h++, pos += virtwinv.iv_w * fact)
				glReadPixels(0, h, winv.iv_w, 1,
				    capture_formats[mode].cf_glmode,
				    GL_UNSIGNED_BYTE,
				    &buf[pos]);

			glMatrixMode(GL_MODELVIEW);
			glutSwapBuffers();
		}

	draw_info("Saving, please wait...");
	glutSwapBuffers();

	if ((fp = fopen(fn, "wb")) == NULL) {
		status_add(SLP_URGENT, "%s", fn);
		goto done;
	}
	capture_formats[mode].cf_writef(fp, buf,
	    virtwinv.iv_w, virtwinv.iv_h);
	fclose(fp);
done:
	free(buf);

	st.st_rf |= RF_CAM;
}
