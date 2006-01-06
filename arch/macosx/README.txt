$Id$

General Notes
=============

The GLUT framework adjusts the working directory in glutInit().

lex and yacc must be run to generate the corresponding .c files
to compile the project in Xcode.  Just run:

	$ make phys-lex.c
	$ make phys-parse.c
