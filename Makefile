# $Id$

PROG = mon
SRCS = draw.c load_png.c mon.c panel.c parse.c
LIBS = -lGL -lglut -lGLU -lpng
CFLAGS += -Wall -W -g

OBJS = ${SRCS:.c=.o}

MAKEOBJDIRPREFIX = obj

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} ${CFLAGS} -c $<

depend:
	mkdep ${SRCS}

clean:
	rm -rf ${PROG} ${OBJS}
