/* $Id$ */

#ifndef _ROUTE_H_
#define _ROUTE_H_

#define RT_RECOVER	0
#define RT_FATAL	1
#define RT_ROUTER	2
#define NRT		3

#define RP_UNK		0
#define RP_NEGX		1
#define RP_POSX		2
#define RP_NEGY		3
#define RP_POSY		4
#define RP_NEGZ		5
#define RP_POSZ		6
#define NRP		7

/* Which "port set" the user is viewing. */
#define RPS_NEG 	0
#define RPS_POS 	1
#define NRPS		2

/*
 * Convert a dimension/set to route port number.
 * Offset by one to account for RP_UNK, which has
 * no port and as such cannot be associated with
 * a dimension.
 */
#define DIM_TO_PORT(dim, set) ((dim) * NRPS + (set) + 1)

/*
 * Each node structure contains a route structure.
 * The route structure contains the number of recoverable,
 * fatal, and router errors for sending receiving to each
 * of its neighbors in all directions.
 */
struct route {
	int rt_err[NRP][NRT];
};

int rteport_to_rdir(int);
int rdir_to_rteport(int);

extern struct route rt_max;
extern struct route rt_zero;

#endif /* _ROUTE_H_ */
