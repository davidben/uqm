
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs \
	@SDL_CFLAGS@ @SMPEG_CFLAGS@

noinst_LTLIBRARIES = libstrings.la
libstrings_la_SOURCES = getstr.c sfileins.c sresins.c strings.c 
noinst_HEADERS = strintrn.h

