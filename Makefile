# $Id$

PROG = mon
SRCS = buf.c cam.c capture.c draw.c flyby.c key.c load_png.c main.c \
    mouse.c panel.c parse.c
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

conn:
	ssh -NfL 3306:localhost:3306 phantom.psc.edu

-include .depend
