/* $Id$ */

struct datasrc;

void		 parse_job(const struct datasrc *);
void		 parse_node(const struct datasrc *);
void		 parse_yod(const struct datasrc *);
void		 parse_rt(const struct datasrc *);
void		 parse_ss(const struct datasrc *);

void		 parse_colors(const char *);
