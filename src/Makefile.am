
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = sc2code

CFLAGS = -Wall

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

bin_PROGRAMS = sc2

sc2_SOURCES = port.c starcon2.c
sc2_LDADD = sc2code/libsc2.la @SDL_LIBS@ @GL_LIBS@ @SMPEG_LIBS@
noinst_HEADERS = port.h

