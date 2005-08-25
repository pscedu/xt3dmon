/* $Id$ */

#include "compat.h"

#include <err.h>
#include <stdlib.h>

#include <png.h>

/*
 * Use libpng to read a PNG file
 * into an newly allocated buffer.
 * (Note: must call free() to dispose!)
 */
void *
png_load(char *file, unsigned int *w, unsigned int *h)
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
	*h = png_get_image_height(ptr, info);
	*w = png_get_image_width(ptr, info);
	//OLD: rowbytes = png_get_rowbytes(ptr, info);

	/* Buffers for the data */
	if ((rows = (malloc(*h * sizeof(png_bytep)))) == NULL)
		err(1, "malloc");

	/* rowbytes/width will tell either RGB or RGBA etc... */
	if ((data = malloc(*w * *h * 4 * sizeof(png_bytep))) == NULL)
		err(1, "malloc");

	/* Set the rows to properly point to our data buffer */
	// OLD: Copy the data into a single buffer */
	if (rows && data) {
		/* Get the data */
		for (i = 0; i < *h; i++)
			rows[*h - 1 - i] = data + (*w * i * 4);

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

/* Save buffer as PNG. */
void
png_write(FILE *fp, unsigned char *buf, long w, long h)
{
	png_structp png;
	png_bytepp rows;
	png_infop info;
	int i;

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
}
