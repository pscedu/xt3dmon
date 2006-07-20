# $Id$

TESTS=		text base64
CFLAGS+=	-I.. -W -Wall
LDFLAGS+=	../text.o ../util.o ../buf.o

all: ${TESTS} xt3dmon

xt3dmon:
	(cd .. && make)
