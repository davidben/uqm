
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

SUBDIRS = ships libs planets comm

# We build the core SC2 code as a static library.

CFLAGS = -W -Wall

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs \
	@SDL_CFLAGS@ @SMPEG_CFLAGS@

noinst_LTLIBRARIES = libsc2.la

libsc2_la_SOURCES = \
	battle.c border.c build.c cleanup.c clock.c collide.c comm.c \
	commglue.c confirm.c credits.c cyborg.c demo.c displist.c \
	dummy.c encount.c fmv.c galaxy.c gameinp.c gameopt.c gendef.c \
	getchar.c globdata.c gravity.c gravwell.c grpinfo.c hyper.c \
	init.c intel.c intro.c ipdisp.c load.c loadship.c master.c \
	melee.c misc.c oscill.c outfit.c pickmele.c pickship.c \
	plandata.c process.c restart.c save.c settings.c setup.c \
	ship.c shipstat.c shipyard.c sis.c starbase.c starcon.c \
	starmap.c state.c status.c tactrans.c trans.c utils.c \
	velocity.c weapon.c 

libsc2_la_LIBADD = \
		ships/libships.la planets/libplanets.la \
		comm/libcomm.la \
		libs/liblibs.la

noinst_HEADERS = \
	appglue.h build.h clock.h coderes.h collide.h commglue.h demo.h \
	displist.h element.h encount.h globdata.h hyper.h ifontres.h \
	igfxres.h ikey_con.h imusicre.h intel.h ires_ind.h isndres.h \
	istrtab.h melee.h nameref.h races.h resinst.h respkg.h \
	restypes.h sis.h sounds.h starbase.h starcon.h state.h \
	units.h velocity.h weapon.h

#noinst_PROGRAMS = sndtest

#sndtest_SOURCES = test.c
#sndtest_LDADD = libsc2.a

