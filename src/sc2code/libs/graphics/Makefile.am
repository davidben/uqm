
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = sdl

CFLAGS = -Wall

INCLUDES = @SDL_CFLAGS@ @SMPEG_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs 

noinst_LTLIBRARIES = libgraphics.la

libgraphics_la_SOURCES = clipline.c context.c drawable.c filegfx.c font.c \
	frame.c intersec.c line.c loaddisp.c makepoly.c map.c \
	pixmap.c poly.c rect.c resgfx.c stamp.c boxint.c gfx_common.c

libgraphics_la_LIBADD = sdl/libsdl.la

#libgraphics_la_LDFLAGS = @SMPEG_LIBS@

noinst_HEADERS = context.h display.h drawable.h font.h gfxintrn.h gfx_common.h

