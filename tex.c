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
	for (i = 0, texid = 1; i < NJST; i++, texid++) {
		if (eggs & EGG_BORG)
			snprintf(path, sizeof(path), _PATH_BORG);
		else
			snprintf(path, sizeof(path), _PATH_TEX, i);

		data = png_load(path, &w, &h);
		tex_init(data, GL_RGBA, GL_RGBA, texid, w, h);
		jstates[i].js_fill.f_texid[wid] = texid;

		texid++;
		data = png_load(path, &w, &h);
		tex_init(data, GL_INTENSITY, GL_RGBA, texid, w, h);
		jstates[i].js_fill.f_texid_a[wid] = texid;
	}

	/* Load the font texture -- background color over white in tex */
	font_id[wid] = texid;
	data = png_load(_PATH_FONT, &w, &h);
	tex_init(data, GL_INTENSITY, GL_RGBA, font_id[wid], w, h);
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
	free(data);
}

/* Delete textures from memory. */
void
tex_remove(void)
{
	int i;

	for (i = 0; i < NJST; i++) {
		if (jstates[i].js_fill.f_texid)
			glDeleteTextures(1, &jstates[i].js_fill.f_texid);
		if (jstates[i].js_fill.f_texid_a)
			glDeleteTextures(1, &jstates[i].js_fill.f_texid_a);
	}
}
