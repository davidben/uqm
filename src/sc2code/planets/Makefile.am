
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

INCLUDES = @SDL_CFLAGS@ \
	-I/usr/include/SDL \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libplanets.la

libplanets_la_SOURCES = \
	cargo.c devices.c genburv.c genchmmr.c gencol.c gendru.c genilw.c \
	genmel.c genmyc.c genorz.c genpet.c genpku.c genrain.c gensam.c \
	genshof.c gensly.c gensol.c genspa.c gensup.c gensyr.c genthrad.c \
	gentopo.c genutw.c genvault.c genvux.c genwreck.c genzoq.c \
	genyeh.c lander.c lander.h lifeform.h oval.c planets.c plangen.c \
	planmap.c planscan.c pl_stuff.c pstarmap.c report.c roster.c \
	scan.c scan.h solarsys.c calc.c orbits.c surface.c

noinst_HEADERS = \
	planets.h elemdata.h plandata.h sundata.h

