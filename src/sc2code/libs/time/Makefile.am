SUBDIRS = sdl

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libtime.la
libtime_la_SOURCES = timecommon.c
libtime_la_LIBADD = sdl/libsdltime.la
noinst_HEADERS = timecommon.h

