
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = opengl pure

CFLAGS = -Wall

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs 

noinst_LTLIBRARIES = libsdl.la

libsdl_la_SOURCES = sdl_common.c rotozoom.c

libsdl_la_LIBADD = opengl/libopengl.la pure/libpure.la

noinst_HEADERS = sdl_common.h rotozoom.h

