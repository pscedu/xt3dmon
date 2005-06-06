/* $Id$ */

#include <sys/param.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <png.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# include <GL/freeglut.h>
#endif

/* Take a screenshot from the current framebuffer (PPM) */
void screenshot_ppm(char *file, int x, int y, int w, int h)
{
	long size;
	unsigned char *buf;
	FILE *fp;

	if ((fp = fopen(file,"wb")) == NULL)
		err(1, "%s", file);

	/* Size = num pixels * 3 bytes per pixel (RGB) */
	size = w*h*3;
	if ((buf = malloc(size * sizeof(unsigned char))) == NULL)
		err(1, "malloc");

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);

	/* Currently uses PPM format */
	fprintf(fp, "P6 %d %d %d\n", w, h, 255);
	fwrite(buf, size, 1, fp);
	fclose(fp);

	free(buf);
	
	// DEBUG
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

/* Convert data to png */
void data2png(char *file, unsigned char *buf, long w, long h)
{
	FILE *fp;
	int i;
	png_structp png;
	png_infop info;
	png_bytepp rows;


	if ((fp = fopen(file,"wb")) == NULL)
		err(1, "%s", file);

	/* Setup PNG file structure */
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png)
		errx(1, "create_write_struct failed");

	info = png_create_info_struct(png);
	if(!info)
	{
		png_destroy_write_struct(&png, (png_infopp)(NULL));
		errx(1, "create_info_struct failed");
	}

	if(setjmp(png_jmpbuf(png)))
	{
		png_destroy_write_struct(&png, (png_infopp)(NULL));
		errx(1, "terminating");
	}

	/* We want io with no zlib compression
	** (highest quality, & fast) */
	png_init_io(png, fp);
	png_set_compression_level(png, Z_NO_COMPRESSION);

	/* Set the header/info */
	png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	/* Pack single byte pixels */
	png_set_packing(png);

	if((rows = malloc(sizeof(png_bytep)*h)) == NULL)
		err(1, "malloc");

	/*
	** Parse into rows [RGBA (4 pix * width)]
	** Note: this takes into account vertical flip
	** use rows[h] then call invert() does the same.
	*/
	for(i = 0; i < h; i++)
		rows[h-i-1] = buf+(i*w*4);

	/* Actaully write the data */
	png_write_image(png, rows);
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);

	free(rows);

	fclose(fp);
}

/* Take a screenshot from the current framebuffer (PNG) */
void screenshot_png(char *file, int x, int y, int w, int h)
{
	long size;
	unsigned char *buf;

	/* Size = num pixels * 4 bytes per pixel (RGBA) */
	size = w*h*4;
	if ((buf = malloc(size * sizeof(unsigned char))) == NULL)
		err(1, "malloc");

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	/* Write data to buf */
	data2png(file, buf, w, h);

	free(buf);
}

/*
** Invert the y pixels because OpenGL
void invert()
{

}
*/

/* Dump Framebuffer into memory write at end */
#define NUM_FRAMES 100
#define MAX_FRAMES 10000
static unsigned char *fb[NUM_FRAMES];

void screenshot_raw(int x, int y, int w, int h)
{
	char file[PATH_MAX];
	long size;
	static int i = 0;
	static int j = 0;

	/*
	** After we reach max frames dump them. NOTE: this is
	** extremely nasty and will probably take awhile...
	*/
	if(i >= NUM_FRAMES)
	{
		int k;
		for(k = 0; k < NUM_FRAMES; k++)
		{
			snprintf(file, sizeof(file), "ppm/%d.png", j++);
			data2png(file, fb[k], w, h);
			free(fb[k]);
		}

		i = 0;
	}

	/* Size = num pixels * 3 bytes per pixel (RGB) */
	size = w*h*4;

	if ((fb[i] = malloc(size * sizeof(unsigned char))) == NULL)
		err(1, "malloc");

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, fb[i]);

	i++;
}

/* Create proper filenames so they are ordered in png->mpg */
void filename(char *ascii, int i)
{
	char num[PATH_MAX];
	int t = i;

	memset(ascii, '\0', sizeof(ascii));

	while(i < MAX_FRAMES)
	{
		i *= 10;
		strcat(ascii, "0");
	}

	snprintf(num, sizeof(num), "%d", t);
	strcat(ascii, num); 

	return ascii;
}

/* Capture Screenshots (buffered) and save as PPM (Portable Pixmaps) */
void screenshot_buf_ppm(int x, int y, int w, int h)
{
	long size;
	FILE *fp;
	char file[PATH_MAX];
	static int i = 0;
	static int j = 1;
	char num[PATH_MAX];
	char cmd[NAME_MAX];
	
	/* Size = num pixels * 3 bytes per pixel (RGB) */
	size = w*h*3;
	
	/*
	** After we reach max frames dump them. NOTE: this is
	** extremely nasty and will probably take awhile...
	*/
	if(i >= NUM_FRAMES)
	{
		int k;
		for(k = 0; k < NUM_FRAMES; k++)
		{
			filename(num, j++);
			snprintf(file, sizeof(file), "ppm/%s.ppm", num);

			if ((fp = fopen(file,"wb")) == NULL)
				err(1, "%s", file);

			/* Currently uses PPM format */
			fprintf(fp, "P6 %d %d %d\n", w, h, 255);
			fwrite(fb[k], size, 1, fp);
			fclose(fp);

			free(fb[k]);
			
			// DEBUG
			memset(cmd, '\0', sizeof(cmd));
			snprintf(cmd, sizeof(cmd),"`which cjpeg` -quality 100 %s | `which jpegtran` -flip vertical > %s.jpg; rm %s", file, file, file);
			system(cmd);
		}

		i = 0;
	}

	if ((fb[i]= malloc(size * sizeof(unsigned char))) == NULL)
		err(1, "malloc");

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, fb[i]);

	i++;
}

/* Dump the entire framebuffer */
void capture_fb(void)
{
	char file[PATH_MAX];
	GLint vp[4];
	static long f = 0;

	memset(file, 0, sizeof(file));
	snprintf(file, sizeof(file), "ppm/%ld.png", f++);

	glMatrixMode(GL_PROJECTION);
	glGetIntegerv(GL_VIEWPORT, vp);
	
//	screenshot_png(file, vp[0], vp[1], vp[2], vp[3]);
//	screenshot_ppm(file, vp[0], vp[1], vp[2], vp[3]);
//	screenshot_raw(vp[0], vp[1], vp[2], vp[3]);
	screenshot_buf_ppm(vp[0], vp[1], vp[2], vp[3]);
	
	glMatrixMode(GL_MODELVIEW);
}
