
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

CFLAGS = -Wall

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs 

noinst_LTLIBRARIES = libsdl.la

libsdl_la_SOURCES = 3do_blt.c 3do_funcs.c 3do_getbody.c dcqueue.c opengl.c \
	primitives.c pure.c rotozoom.c sdl_common.c

noinst_HEADERS = opengl.h primitives.h pure.h rotozoom.h sdl_common.h

