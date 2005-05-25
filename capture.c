#include<stdio.h>
#include<string.h>
#include<png.h>
#include<err.h>

#if defined(__APPLE_CC__)
#include<OpenGL/gl.h>
#include<GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/freeglut.h>
#endif

#define MAX_FILENAME 120000

/* Take a screenshot from the current framebuffer (PPM) */
void screenshot_ppm(char *file, int x, int y, int w, int h)
{
	long size;
	unsigned char *buf;
	FILE *fp;

	fp = fopen(file,"wb");

	if(!fp)
	{
		printf("fopen err\n");
		exit(1);
	}

	/* Size = num pixels * 3 bytes per pixel (RGB) */
	size = w*h*3;
	buf = malloc(size * sizeof(unsigned char));

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);

	/* Currently uses PPM format */
	fprintf(fp, "P6 %d %d %d\n", w, h, 255);
	fwrite(buf, size, 1, fp);
	fclose(fp);
}

/* Take a screenshot from the current framebuffer (PNG) */
void screenshot_png(char *file, int x, int y, int w, int h)
{
	FILE *fp;
	int i;
	long size;
	unsigned char *buf;
	png_structp png;
	png_infop info;
	png_bytepp rows;
	
	fp = fopen(file,"wb");

	if(!fp)
		err(1, "fopen failed");

	/* Size = num pixels * 4 bytes per pixel (RGBA) */
	size = w*h*4;
	buf = malloc(size * sizeof(unsigned char));

	/* Read data from the framebuffer */
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

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
		errx(1, "malloc fail");

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
	free(buf);

	fclose(fp);
}

/*
** Invert the y pixels because OpenGL
void invert()
{

}
*/

/* Dump the entire framebuffer */
void capture_fb(void)
{
	char file[MAX_FILENAME];
	GLint vp[4];
	static long f = 0;

	memset(file, (int)'\0', MAX_FILENAME);
	snprintf(file, MAX_FILENAME, "ppm/%ld.png", f++);

	glMatrixMode(GL_PROJECTION);
	glGetIntegerv(GL_VIEWPORT, vp);
	
	screenshot_png(file, vp[0], vp[1], vp[2], vp[3]);
	
	glMatrixMode(GL_MODELVIEW);
}
