/* $Id$ */

#include <err.h>
#include <stdlib.h>

#ifdef __APPLE_CC__
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif

#include <png.h>

static png_uint_32 width;
static png_uint_32 height;

/*
 * Use libpng to read a PNG file
 * into an newly allocated buffer.
 * (Note: must call free() to dispose!)
 */
void *
load_png(char *file)
{
	unsigned char header[10];
	png_bytepp rows = NULL;
	png_structp ptr = NULL;
	png_infop info = NULL;
	png_infop einfo = NULL;
	png_bytep data = NULL;
	png_uint_32 depth, ctype;
	FILE *fp = NULL;
	size_t i;

	/* Open the PNG in binary mode */
	if ((fp = fopen(file, "rb")) == NULL)
		err(1, "%s", file);

	/* Read the header (first 8 bytes) and check for a valid PNG */
	if (fread(header, 1, 8, fp) != 8) {
		if (ferror(fp))
			err(1, "fread");
		else
			err(1, "read too small");
	}
	header[9] = '\0';

	if (png_sig_cmp((png_bytep)(header), 0, 8))
		errx(1, "%s: invalid PNG file", file);
	/* Create png_struct and png_info structures */
	if ((ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
	    NULL, NULL, NULL)) == NULL)
		errx(1, "png_create_read_struct");

	if ((info = png_create_info_struct(ptr)) == NULL) {
		png_destroy_read_struct(&ptr, NULL, NULL);
		errx(1, "png_create_info_struct");
	}

	if ((einfo = png_create_info_struct(ptr)) == NULL) {
		png_destroy_read_struct(&ptr, &info, NULL);
		errx(1, "png_create_info_struct");
	}

	/*
	 * Set jump points for png errors
	 * (Control returns to this point on error)
	 */
	if (setjmp(png_jmpbuf(ptr))) {
		png_destroy_read_struct(&ptr, &info, &einfo);
		err(1, "setjmp");
	}

	/* Use standard fread() */
	png_init_io(ptr, fp);

	/* Tell it we read the first 8 bytes of header already */
	png_set_sig_bytes(ptr, 8);

	/*
	 * +++++++++++++++++++++++++++++++
	 * FROM HERE STUFF GETS RIDICULOUS
	 *
	 * Somehow these calls below (manual read)
	 * work and the png_read_png() doesn't...
	 * then, instead of using the rowbytes to determine
	 * the array size, it's always 4; I guess this assumes
	 * that there is an alpha channel?
	 */
	png_read_info(ptr, info);

	depth = png_get_bit_depth(ptr, info);
	ctype = png_get_color_type(ptr, info);

	if (ctype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(ptr);

	if (ctype == PNG_COLOR_TYPE_GRAY && depth < 8)
		png_set_gray_1_2_4_to_8(ptr);

	if (ctype == PNG_COLOR_TYPE_GRAY ||
	    ctype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(ptr);

	if (png_get_valid(ptr, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(ptr);
	else
		png_set_filler(ptr, 0xff, PNG_FILLER_AFTER);

	if (depth == 16)
		png_set_strip_16(ptr);

	/* Update what has changed */
	png_read_update_info(ptr, info);

	/* +++++++++++++++++++++++++++++++ */

	/*
	** Read the PNG with no transformation
	** PNG_TRANSFORM_BGR - for BGR & BGRA formats
	*/
	//OLD: png_read_png(ptr, info, PNG_TRANSFORM_IDENTITY, NULL);

	/* Get the size of the image */
	height = png_get_image_height(ptr, info);
	width = png_get_image_width(ptr, info);
	//OLD: rowbytes = png_get_rowbytes(ptr, info);

	/* Buffers for the data */
	if ((rows = (malloc(height * sizeof(png_bytep)))) == NULL)
		err(1, "malloc");

	/* rowbytes/width will tell either RGB or RGBA etc... */
	if ((data = malloc(height*width*4*sizeof(png_bytep))) == NULL)
		err(1, "malloc");

	/* Set the rows to properly point to our data buffer */
	// OLD: Copy the data into a single buffer */
	if (rows && data) {
		/* Get the data */
		for (i = 0; i < height; i++)
			rows[height - 1 - i] = data+(i * 4 *width);

		/* Read the image rows */
		png_read_image(ptr, rows);

		/* They will be stored in data, so free rows */
		free(rows);
	}

	/* Dispose of the read structure */
	png_destroy_read_struct(&ptr, &info, &einfo);

	fclose(fp);
	return (data);
}

void
load_texture(void *data, GLint fmt, int id)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	/* fmt is either GL_INTENSITY, GL_RGBA, ... */
	glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, GL_RGBA,
	    GL_UNSIGNED_BYTE, data);
	
	free(data);
}
