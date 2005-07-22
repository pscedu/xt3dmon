# $Id$

MKDEP = `type -t mkdep >/dev/null 2>&1 && echo mkdep || echo makedepend -f.depend`

PROG = mon
SRCS = buf.c cam.c capture.c db.c draw.c flyby.c key.c job.c \
    load_png.c main.c mouse.c panel.c parse.c tween.c uinp.c widget.c
LIBS = -lGL -lglut -lGLU -lpng `mysql_config --libs` -pg
CFLAGS += -Wall -W -g `mysql_config --cflags | sed "s/'//g"` -pg

OBJS = ${SRCS:.c=.o}

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $<

depend:
	@touch .depend
	${MKDEP} ${CFLAGS} ${SRCS}

clean:
	rm -rf ${PROG} ${OBJS} gmon.out

obj:
	mkdir obj

conn:
	ssh -NfL 3306:sdb:3306 phantom.psc.edu

-include .depend
