# $Id$

MKDEP = `type -t mkdep >/dev/null 2>&1 && echo mkdep || echo makedepend -f.depend`

PROG = xt3dmon
SRCS = buf.c callout.c cam.c capture.c db.c draw.c ds.c eggs.c flyby.c \
    hl.c http.c key.c job.c main.c mouse.c node.c objlist.c panel.c \
    parse.c png.c select.c selnode.c server.c shadow.c status.c tex.c \
    tween.c uinp.c vec.c widget.c
LIBS = -lGL -lglut -lGLU -lpng `mysql_config --libs` -pg
MYSQL_CFLAGS = $$(mysql_config --cflags | sed "s/'//g" | \
    awk '{for (n = 1; n <= NF; n++) if (match($$n, /^-I/)) printf "%s ", $$n }')
CFLAGS += -Wall -W -g ${MYSQL_CFLAGS} -pg
INCS = $$(echo ${CFLAGS} | \
    awk '{for (n = 1; n <= NF; n++) if (match($$n, /^-I/)) printf "%s ", $$n }')

OBJS = ${SRCS:.c=.o}

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $<

depend:
	@touch .depend
	${MKDEP} ${INCS} ${SRCS}

clean:
	rm -rf ${PROG} ${OBJS} gmon.out

obj:
	mkdir obj

conn:
	ssh -NfL 3306:sdb:3306 phantom.psc.edu

-include .depend
