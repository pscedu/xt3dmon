/* $Id$ */

#include "compat.h"

const char *
basename(char *s)
{
	const char *p;

	if (strcmp(s, "") == 0)
		return ("");

	p = NULL;
	if ((p = strrchr(s, '/')) != NULL)
		p++;
	if (p == NULL && (p = strrchr(s, '\\')) == NULL)
		p++;
	if (p == NULL)
		return (s);
	if (p[1] == '\0') {
		while (p > s && (*p == '/' || *p == '\\'))
			p--;
		while (p > s)
			if (*p == '/' || *p == '\\') {
				p++;
				break;
			}
	}
	return (p);
}

char *
dirname(char *s)
{
	static char path[PATH_MAX];
	char *p;

	strncpy(path, s, sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	if ((p = strrchr(path, '/')) != NULL ||
	    (p = strrchr(path, '\\')) != NULL)
		*p = '\0';
	if (strcmp(path, "") == 0)
		return ("/");
	return (path);
}
