
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs \
	@SDL_CFLAGS@

noinst_LTLIBRARIES = libresource.la
libresource_la_SOURCES = getres.c loadres.c resdata.c resinit.c direct.c filecntl.c
noinst_HEADERS = index.h resintrn.h

