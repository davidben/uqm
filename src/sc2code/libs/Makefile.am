SUBDIRS = decomp graphics input math memory resource sound strings \
		task threads time video

noinst_HEADERS = \
	compiler.h declib.h errlib.h gfxlib.h inplib.h mathlib.h \
	memlib.h reslib.h sndlib.h strlib.h tasklib.h threadlib.h \
	timelib.h vargs.h vidlib.h misc.h

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = liblibs.la
liblibs_la_SOURCES = dummy.c
liblibs_la_LIBADD = decomp/libdecomp.la graphics/libgraphics.la \
	input/libinput.la math/libmath.la memory/libmemory.la \
	resource/libresource.la sound/libsound.la \
	strings/libstrings.la task/libtask.la threads/libthreads.la \
	time/libtime.la video/libvideo.la

