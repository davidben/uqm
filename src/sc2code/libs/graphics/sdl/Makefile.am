#CFLAGS =

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs 

noinst_LTLIBRARIES = libsdl.la

libsdl_la_SOURCES = 3do_blt.c 3do_funcs.c 3do_getbody.c dcqueue.c opengl.c \
	primitives.c pure.c rotozoom.c sdl_common.c 2xsai.c

noinst_HEADERS = opengl.h primitives.h pure.h rotozoom.h sdl_common.h \
	dcqueue.h 2xsai.h

