SUBDIRS = sc2code

#CFLAGS =

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

bin_PROGRAMS = sc2

sc2_SOURCES = port.c starcon2.c
sc2_LDADD = sc2code/libsc2.la @SDL_LIBS@ @GL_LIBS@
noinst_HEADERS = port.h

