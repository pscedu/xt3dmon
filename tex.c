/* $Id$ */

#include "compat.h"
#include "mon.h"

/*
 * Load all textures into memory.
 */
void
tex_load(void)
{
	unsigned int i, w, h;
	char path[NAME_MAX];
	void *data;

	/* Read in texture IDs. */
	for (i = 0; i < NJST; i++) {
		if (eggs & EGG_BORG)
			snprintf(path, sizeof(path), _PATH_BORG);
		else
			snprintf(path, sizeof(path), _PATH_TEX, i);
		data = png_load(path, &w, &h);
		tex_init(data, jstates[i].js_fill.f_alpha_fmt,
		    GL_RGBA, i + 1, w, h);
		jstates[i].js_fill.f_texid = i + 1;
	}

	/* Load the font texture */
	font_id = i + 1;
	data = png_load(_PATH_FONT, &w, &h);

	/* This puts background color over white in texture */
	tex_init(data, GL_INTENSITY, GL_RGBA, font_id, w, h);
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

	for (i = 0; i < NJST; i++)
		if (jstates[i].js_fill.f_texid)
			glDeleteTextures(1, &jstates[i].js_fill.f_texid);
}

/* Reload the textures if needed */
void
tex_restore(void)
{
	if (st.st_opts & OP_TEX) {
		int fmt = jstates[0].js_fill.f_alpha_fmt;

		if ((st.st_opts & OP_BLEND && fmt != GL_INTENSITY) ||
		   ((st.st_opts & OP_BLEND) == 0 && fmt != GL_RGBA))
			tex_update();
	}
}

void
tex_update(void)
{
	int j, newfmt;

	newfmt = (jstates[0].js_fill.f_alpha_fmt == GL_RGBA ?
	    GL_INTENSITY : GL_RGBA);
	for (j = 0; j < NJST; j++)
		jstates[j].js_fill.f_alpha_fmt = newfmt;
	st.st_rf |= RF_TEX | RF_CLUSTER;
}
