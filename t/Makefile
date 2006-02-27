# $Id$

TESTS=		text base64
CFLAGS+=	-I..
LDFLAGS+=	../text.o ../util.o

all: ${TESTS} xt3dmon

xt3dmon:
	(cd .. && make)
