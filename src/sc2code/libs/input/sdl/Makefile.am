
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libsdlinput.la
libsdlinput_la_SOURCES = input.c
noinst_HEADERS = input.h

