
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = openal

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libsound.la
libsound_la_SOURCES = fileinst.c resinst.c sound_common.c
libsound_la_LIBADD = openal/libopenalsound.la
noinst_HEADERS = play.h redbook.h sndintrn.h trackplayer.h sound_common.h

