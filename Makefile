# $Id$

PROG = mon
SRCS = mon.c
LIBS = -lGL -lglut

OBJS = ${SRCS:.c=.o}

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${OBJS} > $@

.c.o:
	${CC} ${CFLAGS} ${CFLAGS} -c $<

clean:
	rm -rf ${PROG} ${OBJS}
