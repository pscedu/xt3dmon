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

void ppm_write(const char *, unsigned char *, long, long);
void jpg_write(const char *, unsigned char *, long, long);
void png_write(const char *, unsigned char *, long, long);

struct capture_format {
	const char	 *cf_ext;
	int		  cf_size;
	void		(*cf_writef)(const char *, unsigned char *, long, long);
	int		  cf_glmode;
} capture_formats[] = {
	/* These must correspond to the CM_* constants. */
	{ "png", 4, png_write, GL_RGBA },
	{ "ppm", 3, ppm_write, GL_RGB },
	{ "jpg", 3, jpg_write, GL_RGB }
};

static unsigned char *fbuf[NUM_FRAMES];
static int stereo_left;
static int fbuf_pos;
static int capture_pos;

/* Save buffer as PPM. */
void
ppm_write(const char *fn, unsigned char *buf, long w, long h)
{
	FILE *fp;

	if ((fp = fopen(fn, "wb")) == NULL)
		err(1, "%s", fn);
	fprintf(fp, "P6 %ld %ld %d\n", w, h, 255);
	fwrite(buf, 3 * w * h, 1, fp);
	fclose(fp);
}

/* Save buffer as JPEG. */
void
jpg_write(const char *fn, unsigned char *buf, long w, long h)
{
	char cmd[BUFSIZ];

	ppm_write(fn, buf, w, h);

	/* XXX:  DEBUG (definitely remove later!) */
	memset(cmd, '\0', sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "cjpeg -quality 100 %s | "
	    "jpegtran -flip vertical > %s.jpg; rm %s",
	    fn, fn, fn);
	system(cmd);
}

/* Save buffer as PNG. */
void
png_write(const char *fn, unsigned char *buf, long w, long h)
{
	png_structp png;
	png_bytepp rows;
	png_infop info;
	FILE *fp;
	int i;

	if ((fp = fopen(fn, "wb")) == NULL)
		err(1, "%s", fn);

	/* Setup PNG file structure. */
	if ((png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	    NULL, NULL)) == NULL)
		errx(1, "png_create_write_struct failed");

	if ((info = png_create_info_struct(png)) == NULL) {
		png_destroy_write_struct(&png, NULL);
		errx(1, "png_create_info_struct failed");
	}

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, NULL);
		err(1, "setjmp");
	}

	/* We want I/O without zlib compression (highest quality & fast). */
	png_init_io(png, fp);
//	png_set_compression_level(png, Z_NO_COMPRESSION);
	png_set_compression_level(png, 4);
	png_set_compression_mem_level(png, 8);
//	png_set_compression_strategy(png, Z_DEFAULT_STRATEGY);

	/* Set the header/info. */
	png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
	    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	    PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	/* Pack single byte pixels. */
	png_set_packing(png);

	if ((rows = malloc(sizeof(png_bytep) * h)) == NULL)
		err(1, "malloc");

	/*
	 * Parse into rows [RGBA (4 pix * width)]
	 * Note: this takes into account vertical flip
	 * use rows[h] then call invert() does the same.
	 */
	for (i = 0; i < h; i++)
		rows[h - i - 1] = buf + (i * w * 4);

	/* Actaully write the data. */
	png_write_image(png, rows);
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);

	free(rows);
	fclose(fp);
}

void
capture_writeback(int mode)
{
	char fn[PATH_MAX];
	const char *ext;
	int k;

	ext = capture_formats[mode].cf_ext;
	for (k = 0; k < fbuf_pos; k++, capture_pos++) {
		if (stereo) {
			snprintf(fn, sizeof(fn), "%s/%c%07d.%s",
			    _PATH_SSDIR, stereo_left ? 'l' : 'r',
			    capture_pos / 2, ext);
			stereo_left = !stereo_left;
		} else
			snprintf(fn, sizeof(fn), "%s/%07d.%s",
			    _PATH_SSDIR, capture_pos, ext);
		capture_formats[mode].cf_writef(fn, fbuf[k],
		    win_width, win_height);
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
	if (stereo) {
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

	size = capture_formats[mode].cf_size *
	    win_width * win_height;
	if ((buf = realloc(buf, size * sizeof(*buf))) == NULL)
		err(1, "realloc");
	capture_copyfb(mode, buf);

	/* Write data to buffer. */
	capture_formats[mode].cf_writef(fn, buf, win_width, win_height);
}
