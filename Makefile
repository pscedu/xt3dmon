# $Id$

PROG = mon
SRCS = mon.c parse.o
LIBS = -lGL -lglut -lGLU
CFLAGS += -Wall

OBJS = ${SRCS:.c=.o}

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} ${CFLAGS} -c $<

clean:
	rm -rf ${PROG} ${OBJS}
