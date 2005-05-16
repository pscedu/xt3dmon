# $Id$

PROG = mon
SRCS = mon.c parse.c load_png.c
LDFLAGS = -lGL -lglut -lGLU -lpng
CFLAGS += -Wall

.include <bsd.prog.mk>
