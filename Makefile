# $Id$

MKDEP = `type -t mkdep >/dev/null 2>&1 && echo mkdep || echo makedepend -f.depend`

PROG = xt3dmon

SRCS = arch.c buf.c callout.c cam.c capture.c dbg.c deusex.c draw.c ds.c
SRCS+= dx-lex.l dx-parse.y
SRCS+= eggs.c env.c fill.c flyby.c gl.c hl.c http.c job.c key.c lnseg.c
SRCS+= main.c mark.c math.c mouse.c node.c objlist.c panel.c parse.c
SRCS+= phys-lex.l phys-parse.y png.c prefresh.c reel.c select.c
SRCS+= selnode.c server.c shadow.c ssl.c status.c tex.c text.c tween.c
SRCS+= uinp.c ustrdtab.c ustream.c ustrop-file.c ustrop-ssl.c util.c
SRCS+= vec.c widget.c yod.c

LIBS += -lglut -lglx -lGL -lGLU -lXext -lglut -lX11 -lpng -lssl
LIBS += -lm -lcrypto -lkrb5 -lk5crypto -lz -lcom_err -lpthread
LIBS += -lresolv -ldl -lXxf86vm

CFLAGS += -Wall -W -g -D_LIVE_DSP=DSP_LOCAL
#CFLAGS += -O -Wuninitialized
LDFLAGS += -L/usr/X11R6/lib/modules/extensions -L/usr/X11R6/lib
LDFLAGS +=

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

obj:
	mkdir obj

lines:
	@shopt -s extglob && eval 'wc -l !(phys-parse|dx-parse).h \
	    !(phys-parse|phys-lex|dx-parse|dx-lex).c *.y *.l' | tail -1

run: xt3dmon
	./xt3dmon -N

debug: xt3dmon
	gdb -q ./xt3dmon

macdirs:
	cp -R img data arch/macosx/build/Development/xt3dmon.app/Contents/Resources

-include .depend
