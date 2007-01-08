# $Id$

TESTS=		text fmt_scaled base64
CFLAGS+=	-I.. -W -Wall
LDFLAGS+=	../text.o ../util.o ../buf.o -lm

all: ${TESTS} xt3dmon

xt3dmon:
	(cd .. && make)
