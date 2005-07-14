# $Id$

MKDEP = `type -t makedepend >/dev/null 2>&1 && echo makedepend -f.depend || echo mkdep`

PROG = mon
SRCS = buf.c cam.c capture.c db.c draw.c flyby.c key.c load_png.c \
    main.c mouse.c panel.c parse.c
LIBS = -lGL -lglut -lGLU -lpng `mysql_config --libs` -pg
CFLAGS += -Wall -W -g `mysql_config --include | sed "s/'//g"` -pg
INCLUDE += `mysql_config --include | sed "s/'//g"`

OBJS = ${SRCS:.c=.o}

MAKEOBJDIRPREFIX = obj

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $<

depend:
	@touch .depend
	${MKDEP} ${INCLUDE} ${SRCS}

clean:
	rm -rf ${PROG} ${OBJS}

conn:
	ssh -NfL 3306:sdb:3306 phantom.psc.edu

-include .depend
