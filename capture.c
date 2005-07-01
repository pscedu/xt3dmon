/* $Id$ */

#include <sys/param.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include "mon.h"

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

#define NUM_FRAMES 200
#define MAX_FRAMES 25000
static unsigned char *fbuf[NUM_FRAMES];

/* Convert data to png */
void
data2png(char *file, unsigned char *buf, long w, long h)
{
	FILE *fp;
	int i;
	png_structp png;
	png_infop info;
	png_bytepp rows;

	if ((fp = fopen(file,"wb")) == NULL)
		err(1, "%s", file);

	/* Setup PNG file structure */
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

/* Take a screenshot from the current framebuffer (PNG). */
void
ss_png(char *file, int x, int y, int w, int h)
{
	unsigned char *buf;
	long size;

	/* num pixels * 4 bytes per pixel (RGBA). */
	size = w * h * 4;
	if ((buf = malloc(size * sizeof(unsigned char))) == NULL)
		err(1, "malloc");

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	/* Write data to buf */
	data2png(file, buf, w, h);

	free(buf);
}

/* Take a screenshot from the current framebuffer (PPM). */
void
ss_ppm(char *file, int x, int y, int w, int h)
{
	unsigned char *buf;
	long size;
	FILE *fp;

	if ((fp = fopen(file, "wb")) == NULL)
		err(1, "%s", file);

	/* Size = num pixels * 3 bytes per pixel (RGB) */
	size = w * h * 3;
	if ((buf = malloc(size * sizeof(unsigned char))) == NULL)
		err(1, "malloc");

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);

	/* Currently uses PPM format */
	fprintf(fp, "P6 %d %d %d\n", w, h, 255);
	fwrite(buf, size, 1, fp);
	fclose(fp);

	free(buf);

	// DEBUG - Pipe to JPEG Tools
//	memset(cmd, '\0', sizeof(cmd));
//	snprintf(cmd, sizeof(cmd),"/usr/local/bin/cjpeg -quality 100 %s | /usr/local/bin/jpegtran -flip vertical > %s.jpg; rm %s", file, file, file);
//	system(cmd);
}

/*
** Invert the y pixels because OpenGL
void invert()
{

}
*/

/* Dump Framebuffer into memory write at end */
void
fb_mem_png(int x, int y, int w, int h)
{
	char file[PATH_MAX];
	static int i = 0;
	static int j = 1;
	int k;

	/*
	 * After we reach max frames, dump them.  NOTE: this is
	 * extremely nasty and will probably take awhile...
	 */
	if (i >= NUM_FRAMES) {
		for (k = 0; k < NUM_FRAMES; k++) {
			snprintf(file, sizeof(file), "ppm/%0*d.png",
			    (int)ceilf(log10f(MAX_FRAMES)), j++);
			data2png(file, fbuf[k], w, h);
		}
		i = 0;
	}

	/* Read data from the framebuffer. */
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, fbuf[i]);
	i++;
}

/* Capture screenshots (buffered) and save as PPM (portable pixmaps). */
void
fb_mem_ppm(int x, int y, int w, int h)
{
	char file[PATH_MAX], cmd[NAME_MAX];
	static int i = 0;
	static int j = 1;
	long size;
	FILE *fp;
	int k;

	/* num pixels * 3 bytes per pixel (RGB). */
	size = w * h * 3;

	/*
	 * After we reach max frames, dump them.  NOTE: this is
	 * extremely nasty and will probably take awhile...
	 */
	if (i >= NUM_FRAMES) {
		for (k = 0; k < NUM_FRAMES; k++) {
			snprintf(file, sizeof(file), "ppm/%0*d.ppm",
			    (int)ceilf(log10f(MAX_FRAMES)), j++);

			if ((fp = fopen(file, "wb")) == NULL)
				err(1, "%s", file);

			/* Currently uses PPM format */
			fprintf(fp, "P6 %d %d %d\n", w, h, 255);
			fwrite(fbuf[k], size, 1, fp);
			fclose(fp);

			// DEBUG (definately remove later!)
			memset(cmd, '\0', sizeof(cmd));
			snprintf(cmd, sizeof(cmd),
			    "cjpeg -quality 100 %s | "
			    "jpegtran -flip vertical > %s.jpg; rm %s",
			    file, file, file);
			system(cmd);
		}

		i = 0;
	}

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, fbuf[i]);

	i++;
}

void
begin_capture(int mode)
{
	int i;
	GLint vp[4];
	long size = 0;

	glMatrixMode(GL_PROJECTION);
	glGetIntegerv(GL_VIEWPORT, vp);
	glMatrixMode(GL_MODELVIEW);

	/* RGB(A?) */
	size = vp[2] * vp[3] * mode;

	for(i = 0; i < NUM_FRAMES; i++)
		if ((fbuf[i] = malloc(size * sizeof(unsigned char))) == NULL)
			err(1, "malloc");
}

void
end_capture(void)
{
	int i;

	for (i = 0; i < NUM_FRAMES; i++)
		free(fbuf[i]);
}


/* Start capturing frames from the framebuffer */
void
capture_fb(int mode)
{
	GLint vp[4];

	glMatrixMode(GL_PROJECTION);
	glGetIntegerv(GL_VIEWPORT, vp);

	if (mode == PNG_FRAMES)
		fb_mem_png(vp[0], vp[1], vp[2], vp[3]);
	else
		fb_mem_ppm(vp[0], vp[1], vp[2], vp[3]);

	glMatrixMode(GL_MODELVIEW);
}

/* Take a screenshot in png format */
void
screenshot(char *file, int mode)
{
	GLint vp[4];

	glMatrixMode(GL_PROJECTION);
	glGetIntegerv(GL_VIEWPORT, vp);

	if (mode == PNG_FRAMES)
		ss_png(file, vp[0], vp[1], vp[2], vp[3]);
	else
		ss_ppm(file, vp[0], vp[1], vp[2], vp[3]);

	glMatrixMode(GL_MODELVIEW);
}
