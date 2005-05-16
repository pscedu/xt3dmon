# $Id$

PROG = mon
SRCS = mon.c parse.c load_png.c
LIBS = -lGL -lglut -lGLU -lpng
CFLAGS += -Wall

OBJS = ${SRCS:.c=.o}

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} ${CFLAGS} -c $<

depend:
	mkdep ${SRCS}

clean:
	rm -rf ${PROG} ${OBJS}
