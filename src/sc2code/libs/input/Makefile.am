
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = sdl

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libinput.la
libinput_la_SOURCES = input_common.c
libinput_la_LIBADD = sdl/libsdlinput.la
noinst_HEADERS = input_common.h inpintrn.h

