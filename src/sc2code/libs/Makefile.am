
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = decomp graphics input math memory resource sound strings video

noinst_HEADERS = \
	compiler.h declib.h errlib.h gfxlib.h inplib.h mathlib.h \
	memlib.h reslib.h sndlib.h strlib.h timlib.h vargs.h vidlib.h

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = liblibs.la
liblibs_la_SOURCES = dummy.c
liblibs_la_LIBADD = decomp/libdecomp.la graphics/libgraphics.la \
	input/libinput.la math/libmath.la memory/libmemory.la \
	resource/libresource.la sound/libsound.la \
	strings/libstrings.la video/libvideo.la

