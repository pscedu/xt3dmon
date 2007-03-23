# $Id$

MKDEP = `type -t mkdep >/dev/null 2>&1 && echo mkdep || echo makedepend -f.depend`
LINT = splint -posix-lib -nestcomment -retvalint -nullstate -mustfreeonly -D__GNUC__ -D_GNU_SOURCE
CTAGS = ctags

PROG = xt3dmon

SRCS = arch.c
SRCS+= buf.c
SRCS+= callout.c
SRCS+= cam.c
SRCS+= capture.c
SRCS+= dbg.c
SRCS+= deusex.c
SRCS+= draw.c
SRCS+= ds.c
SRCS+= dx-lex.l
SRCS+= dx-parse.y
SRCS+= eggs.c
SRCS+= env.c
SRCS+= fill.c
SRCS+= flyby.c
SRCS+= gl.c
SRCS+= gss.c
SRCS+= hl.c
SRCS+= http.c
SRCS+= job.c
SRCS+= key.c
SRCS+= lnseg.c
SRCS+= mach-lex.l
SRCS+= mach-parse.y
SRCS+= mach.c
SRCS+= main.c
SRCS+= mark.c
SRCS+= math.c
SRCS+= mouse.c
SRCS+= node.c
SRCS+= objlist.c
SRCS+= panel.c
SRCS+= parse.c
SRCS+= png.c
SRCS+= prefresh.c
SRCS+= reel.c
SRCS+= rte.c
SRCS+= select.c
SRCS+= selnid.c
SRCS+= selnode.c
SRCS+= server.c
SRCS+= shadow.c
SRCS+= ssl.c
SRCS+= status.c
SRCS+= tex.c
SRCS+= text.c
SRCS+= tween.c
SRCS+= uinp.c
SRCS+= ustrdtab.c
SRCS+= ustream.c
SRCS+= ustrop-file.c
SRCS+= ustrop-gzip.c
SRCS+= ustrop-ssl.c
SRCS+= util.c
SRCS+= vec.c
SRCS+= widget.c
SRCS+= yod.c

CFLAGS += -Wall -W -g -pipe
CFLAGS += -Wno-parentheses -Wshadow -Wredundant-decls
CFLAGS += -D_GSS
CFLAGS += -D_LIVE_PROTO=\"file\" -D_LIVE_PATH=_PATH_DATA
#CFLAGS += -Wconversion
CFLAGS += -DYY_NO_UNPUT
#CFLAGS += -O3 -Wuninitialized -fomit-frame-pointer
#CFLAGS += -fno-strict-aliasing
CFLAGS += -I/usr/X11R6/include

LIBS += -lGL -lGLU -lglut -lssl -lpng -lz

# static compiles:
# LIBS += -lGL -lGLU -lglut -lssl -lpng12 -lz -lm -lgssapi_krb5
# LIBS += -lcrypto -lXext -lX11 -ldl -lXxf86vm -lkrb5
# LIBS += -lcom_err -lk5crypto -lresolv -lkrb5support -lpthread
# LDFLAGS += -static -L/usr/X11R6/lib/modules/extensions

LDFLAGS += -L/usr/X11R6/lib

YFLAGS += -d -b "$$(echo $@ | sed 's/-.*//')" \
	     -p "$$(echo $@ | sed 's/-.*//')" \
	     -o $@
LFLAGS += -P"$$(echo $@ | sed 's/-.*//')"

INCS = $$(echo ${CFLAGS} | \
    awk '{for (n = 1; n <= NF; n++) if (match($$n, /^-I/)) printf "%s ", $$n }')
INCS += $$(if cc -v 2>&1 | grep -q gcc; then cc -print-search-dirs | \
    grep install | awk '{print "-I" $$2 "include"}' | sed 's/:/ -I/'; fi)

OBJS = $(patsubst %.c,%.o,$(filter %.c,${SRCS}))
OBJS+= $(patsubst %.y,%.o,$(filter %.y,${SRCS}))
OBJS+= $(patsubst %.l,%.o,$(filter %.l,${SRCS}))

CSRCS = $(filter %.c,${SRCS})
CSRCS+= $(patsubst %.y,%.c,$(filter %.y,${SRCS}))
CSRCS+= $(patsubst %.l,%.c,$(filter %.l,${SRCS}))

CLEAN+= gmon.out dx-lex.c dx-parse.c dx-parse.h dx-parse.output
CLEAN+= mach-lex.c mach-parse.c mach-parse.h mach-parse.output

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

build:
	${MAKE} clean && ${MAKE} depend && ${MAKE} all

lint:
	${LINT} ${INCS} ${CSRCS}

tags: ${SRCS}
	${CTAGS} ${SRCS}

lines:
	@shopt -s extglob && eval 'wc -l !(mach-parse|dx-parse).h \
	    !(mach-parse|mach-lex|dx-parse|dx-lex).c *.y *.l' | tail -1

run: ${PROG}
	./${PROG} -M

# -gldebug
debug: ${PROG}
	gdb -q ./${PROG}

DIST_LN_DIRS = img data scripts
DIST_DIRS = snaps
MACOS_DIR = arch/macosx/build/Development/xt3dmon.app/Contents/Resources

macdirs:
	for i in ${DIST_CP_DIRS}; do rm -rf ${MACOS_DIR}/$$i; done
	mkdir -p ${MACOS_DIR}
	for i in ${DIST_DIRS}; do mkdir -p ${MACOS_DIR}/$$i; done
	for i in ${DIST_LN_DIRS}; do						\
		[ -e ${MACOS_DIR}/$$i ] || ln -s `pwd`/$$i ${MACOS_DIR};	\
	done

-include .depend
