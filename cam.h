/* $Id$ */
/*
 * %PSC_START_COPYRIGHT%
 * -----------------------------------------------------------------------------
 * Copyright (c) 2005-2010, Pittsburgh Supercomputing Center (PSC).
 *
 * Permission to use, copy, and modify this software and its documentation
 * without fee for personal use or non-commercial use within your organization
 * is hereby granted, provided that the above copyright notice is preserved in
 * all copies and that the copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to other
 * organizations or individuals is not permitted without the written permission
 * of the Pittsburgh Supercomputing Center.  PSC makes no representations about
 * the suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * -----------------------------------------------------------------------------
 * %PSC_END_COPYRIGHT%
 */

struct fvec;

void	 cam_move(int, double);
void	 cam_revolve(struct fvec *, int, double, double, int);
void	 cam_revolvefocus(double, double);
void	 cam_rotate(const struct fvec *, const struct fvec *, int, int);
void	 cam_roll(double);
void	 cam_look(void);
void	 cam_calcuv(struct fvec *);
void	 cam_bird(int);

#define REVT_LKAVG	0	/* Average look across center. */
#define REVT_LKCEN	1	/* Always look at center. */
#define REVT_LKFIT	2	/* Fit all points. */
