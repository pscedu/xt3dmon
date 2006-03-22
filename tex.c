/* $Id$ */

#include "mon.h"

#include <stdlib.h>

#include "nodeclass.h"
#include "pathnames.h"
#include "env.h"

/*
 * "Initialize" texture data that are loaded into memory.
 * ifmt is internal format: GL_INTENSITY, GL_RGBA, ...
 */
unsigned int
tex_init(void *data, int ifmt, unsigned int w, unsigned int h)
{
	static unsigned int id;
	int nd;

	nd = GL_TEXTURE_2D;
	glGenTextures(1, &id);
	glBindTexture(nd, id);

	glTexParameteri(nd, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(nd, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(nd, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(nd, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(nd, 0, ifmt, w, h, 0, GL_RGBA,
	    GL_UNSIGNED_BYTE, data);
	return (id++);
}

/*
 * Load all textures into memory.
 */
void
tex_load(void)
{
	unsigned int i, w, h;
	char path[NAME_MAX];
	void *data;

	/* Status class textures. */
	for (i = 0; i < NSC; i++) {
		snprintf(path, sizeof(path), _PATH_TEX_SC, i);
		data = png_load(path, &w, &h);
		statusclass[i].nc_fill.f_texid[wid] = tex_init(data, GL_RGBA, w, h);
		statusclass[i].nc_fill.f_texid_a[wid] = tex_init(data, GL_INTENSITY, w, h);
		free(data);
	}

	/* Load font texture -- always alpha-blended. */
	data = png_load(_PATH_FONT, &w, &h);
	fill_font.f_texid_a[wid] = tex_init(data, GL_INTENSITY, w, h);
	free(data);

	/* Borg texture. */
	data = png_load(_PATH_TEX_BORG, &w, &h);
	fill_borg.f_texid[wid] = tex_init(data, GL_RGBA, w, h);
	fill_borg.f_texid_a[wid] = tex_init(data, GL_INTENSITY, w, h);
	free(data);

	/* Selnode texture. */
	data = png_load(_PATH_TEX_SN, &w, &h);
	fill_selnode.f_texid[wid] = tex_init(data, GL_RGBA, w, h);
	fill_selnode.f_texid_a[wid] = tex_init(data, GL_INTENSITY, w, h);
	free(data);
}

/* Remove textures from memory. */
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
