/* $Id$ */

#include "compat.h"
#include "mon.h"

/*
 * Load all textures into memory.
 */
void
tex_load(void)
{
	unsigned int i, w, h, texid;
	char path[NAME_MAX];
	void *data;

	/* Read in texture IDs. */
	for (i = 0, texid = 1; i < NSC; i++, texid++) {
		snprintf(path, sizeof(path), _PATH_TEX, i);
		data = png_load(path, &w, &h);
		tex_init(data, GL_RGBA, GL_RGBA, texid, w, h);
		statusclass[i].nc_fill.f_texid[wid] = texid;

		texid++;
		tex_init(data, GL_INTENSITY, GL_RGBA, texid, w, h);
		statusclass[i].nc_fill.f_texid_a[wid] = texid;
		free(data);
	}

	/* Load the font texture -- background color over white in tex */
	data = png_load(_PATH_FONT, &w, &h);
	tex_init(data, GL_INTENSITY, GL_RGBA, texid, w, h);
	fill_font.f_texid_a[wid] = texid;
	free(data);

	texid++;
	data = png_load(_PATH_BORG, &w, &h);
	tex_init(data, GL_RGBA, GL_RGBA, texid, w, h);
	fill_borg.f_texid[wid] = texid;

	texid++;
	tex_init(data, GL_INTENSITY, GL_RGBA, texid, w, h);
	fill_borg.f_texid_a[wid] = texid;
	free(data);
}

/*
 * "Initialize" texture data that are loaded into memory.
 */
void
tex_init(void *data, GLint ifmt, GLenum fmt, GLuint id, GLuint w, GLuint h)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* fmt is either GL_INTENSITY, GL_RGBA, ... */
	glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt,
	    GL_UNSIGNED_BYTE, data);
}

/* Delete textures from memory. */
void
tex_remove(void)
{
	struct fill *fp;
	int i;

	for (i = 0; i < NSC; i++) {
		fp = &statusclass[i].nc_fill;
		if (fp->f_texid[wid])
			glDeleteTextures(1, &fp->f_texid[wid]);
		if (fp->f_texid_a[wid])
			glDeleteTextures(1, &fp->f_texid_a[wid]);
	}
}
