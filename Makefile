# $Id$

MKDEP = `type -t mkdep >/dev/null 2>&1 && echo mkdep || echo makedepend -f.depend`

PROG = xt3dmon

SRCS = arch.c buf.c callout.c cam.c capture.c dbg.c deusex.c draw.c ds.c
SRCS+= dx-lex.l dx-parse.y eggs.c env.c fill.c flyby.c gl.c gss.c hl.c
SRCS+= http.c job.c key.c lnseg.c main.c mark.c math.c mouse.c node.c
SRCS+= objlist.c panel.c parse.c phys-lex.l phys-parse.y png.c
SRCS+= prefresh.c reel.c rte.c select.c selnode.c server.c shadow.c
SRCS+= ssl.c status.c tex.c text.c tween.c uinp.c ustrdtab.c ustream.c
SRCS+= ustrop-file.c ustrop-ssl.c util.c vec.c widget.c yod.c

CFLAGS += -Wall -W -g -D_GSS -D_LIVE_DSP=DSP_LOCAL
#CFLAGS += -Wconversion
CFLAGS += -O3 -Wuninitialized -fomit-frame-pointer
CFLAGS += -fno-strict-aliasing

LIBS += -lGL -lGLU -lglut -lssl -lpng -lz

# static compiles:
# LIBS += -lglut -lglx -lGL -lGLU -lXext -lglut -lX11 -lpng -lssl
# LIBS += -lm -lcrypto -lkrb5 -lk5crypto -lz -lcom_err -lpthread
# LIBS += -lresolv -ldl -lXxf86vm
# LDFLAGS += -L/usr/X11R6/lib/modules/extensions

LDFLAGS += -L/usr/X11R6/lib

YFLAGS += -d -b "$$(echo $@ | sed 's/-.*//')" \
	     -p "$$(echo $@ | sed 's/-.*//')" \
	     -o $@
LFLAGS += -P"$$(echo $@ | sed 's/-.*//')"

INCS = $$(echo ${CFLAGS} | \
    awk '{for (n = 1; n <= NF; n++) if (match($$n, /^-I/)) printf "%s ", $$n }')

OBJS = $(patsubst %.c,%.o,$(filter %.c,${SRCS}))
OBJS+= $(patsubst %.y,%.o,$(filter %.y,${SRCS}))
OBJS+= $(patsubst %.l,%.o,$(filter %.l,${SRCS}))

CSRCS = $(filter %.c,${SRCS})
CSRCS+= $(patsubst %.y,%.c,$(filter %.y,${SRCS}))
CSRCS+= $(patsubst %.l,%.c,$(filter %.l,${SRCS}))

CLEAN+= gmon.out dx-lex.c dx-parse.c dx-parse.h
CLEAN+= phys-lex.c phys-parse.c phys-parse.h

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} -o $@ ${LIBS}

.y.c:
	${YACC} ${YFLAGS} $<

.c.o:
	${CC} ${CFLAGS} -c $<

depend: ${CSRCS}
	@touch .depend
	${MKDEP} ${INCS} ${CSRCS}

clean:
	rm -rf ${PROG} ${OBJS} ${CLEAN}

lint:

lines:
	@shopt -s extglob && eval 'wc -l !(phys-parse|dx-parse).h \
	    !(phys-parse|phys-lex|dx-parse|dx-lex).c *.y *.l' | tail -1

run: xt3dmon
	./xt3dmon

debug: xt3dmon
	gdb -q ./xt3dmon

DIST_DIRS = img data snaps scripts
MACOS_DIR = arch/macosx/build/Development/xt3dmon.app/Contents/Resources

macdirs:
	for i in ${DIST_DIRS}; do rm -rf ${MACOS_DIR}/$$i; done
	mkdir -p ${MACOS_DIR}
	cp -R ${DIST_DIRS} ${MACOS_DIR}

-include .depend
