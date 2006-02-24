# $Id$

MKDEP = `type -t mkdep >/dev/null 2>&1 && echo mkdep || echo makedepend -f.depend`

PROG = xt3dmon

SRCS = arch.c buf.c callout.c cam.c capture.c dbg.c draw.c ds.c eggs.c
SRCS+= flyby.c gl.c hl.c http.c job.c key.c main.c math.c mouse.c
SRCS+= node.c objlist.c panel.c parse.c phys-lex.l phys-parse.y png.c
SRCS+= select.c selnode.c server.c shadow.c status.c tex.c text.c
SRCS+= tween.c uinp.c ustrdtab.c ustream.c ustrop-file.c ustrop-ssl.c
SRCS+= ustrop-winsock.c vec.c widget.c yod.c

LIBS = -lGL -lglut -lGLU -lpng -pg
CFLAGS += -Wall -W -g -pg -D_LIVE_DSP=DSP_LOCAL
# CFLAGS += -O -Wuninitialized
YFLAGS += -d
INCS = $$(echo ${CFLAGS} | \
    awk '{for (n = 1; n <= NF; n++) if (match($$n, /^-I/)) printf "%s ", $$n }')

OBJS = $(patsubst %.c,%.o,$(filter %.c,${SRCS}))
OBJS+= $(patsubst %.y,%.o,$(filter %.y,${SRCS}))
OBJS+= $(patsubst %.l,%.o,$(filter %.l,${SRCS}))

CSRCS = $(filter %.c,${SRCS})
CSRCS+= $(patsubst %.y,%.c,$(filter %.y,${SRCS}))
CSRCS+= $(patsubst %.l,%.c,$(filter %.l,${SRCS}))

CLEAN+= phys-lex.c phys-parse.c y.tab.h gmon.out

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $<

depend: ${CSRCS}
	@touch .depend
	${MKDEP} ${INCS} ${CSRCS}

clean:
	rm -rf ${PROG} ${OBJS} ${CLEAN}

obj:
	mkdir obj

-include .depend
