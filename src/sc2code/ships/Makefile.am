
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

RACES = androsyn arilou blackurq chenjesu chmmr druuge human ilwrath \
	lastbat melnorme mmrnmhrm mycon orz pkunk probe shofixti \
	sis_ship slylandr spathi supox syreen thradd umgah urquan utwig \
	vux yehat zoqfot

EXTRA_DIRS = $(RACES)

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libships.la
libships_la_LIBADD = $(patsubst %.c, %.lo, $(wildcard */*.c))
libships_la_SOURCES = dummy.c

%.lo: %.c
	$(COMPILE) -c $< -o $@

clean: clean-am
	rm -f */*.{o,lo}

distclean: distclean-am
	rm -f */*.{o,lo}


