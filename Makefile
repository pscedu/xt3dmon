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

LIBS = -lGL -lglut -lGLU -lpng -lssl -lm -lcrypto -lcom_err
# -lGLcore -lXext -lX11 -ldl -lXxf86vm -lgss_s -lz
# -lgssapi_krb5 -lkrb5 -lk5crypto -lresolv

CFLAGS += -Wall -W -g -D_LIVE_DSP=DSP_LOCAL
#CFLAGS += -O -Wuninitialized
LDFLAGS =

YFLAGS += -d -b "$$(echo $@ | sed 's/-.*//')" \
	     -p "$$(echo $@ | sed 's/-.*//')" \
	     -o $@

INCS = $$(echo ${CFLAGS} | \
    awk '{for (n = 1; n <= NF; n++) if (match($$n, /^-I/)) printf "%s ", $$n }')

OBJS = $(patsubst %.c,%.o,$(filter %.c,${SRCS}))
OBJS+= $(patsubst %.y,%.o,$(filter %.y,${SRCS}))
OBJS+= $(patsubst %.l,%.o,$(filter %.l,${SRCS}))

CSRCS = $(filter %.c,${SRCS})
CSRCS+= $(patsubst %.y,%.c,$(filter %.y,${SRCS}))
CSRCS+= $(patsubst %.l,%.c,$(filter %.l,${SRCS}))

CLEAN+= dx-parse.c phys-lex.c phys-parse.c y.tab.h gmon.out

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} ${LIBS} ${OBJS} -o $@

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
	@shopt -s extglob && eval \
	    'wc -l !(y.tab).h !(phys-parse|phys-lex).c *.y *.l' | tail -1

run: xt3dmon
	./xt3dmon

debug: xt3dmon
	gdb -q ./xt3dmon

macdirs:
	cp -R img data arch/macosx/build/Development/xt3dmon.app/Contents/Resources

-include .depend
