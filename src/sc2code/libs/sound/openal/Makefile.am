
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libopenalsound.la
libopenalsound_la_SOURCES = music.c sfx.c sound.c stream.c trackplayer.c

noinst_HEADERS = sound.h stream.h

