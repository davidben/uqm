SUBDIRS = sdl

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libthreads.la
libthreads_la_SOURCES = thrcommon.c
libthreads_la_LIBADD = sdl/libsdlthreads.la
noinst_HEADERS = thrcommon.h

