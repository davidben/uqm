
# Automake template file.
# Written by Mudry <mudry@user.sf.net>, protected by GNU GPL.
# CVS $ Id: $

RACES = \
	arilou blackur chmmr comandr druuge ilwrath melnorm mycon orz pkunk \
	rebel shofixt slyhome slyland spahome spathi starbas supox syreen \
	talkpet thradd umgah urquan utwig vux yehat zoqfot

EXTRA_DIRS = $(RACES)

INCLUDES = @SDL_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sc2code \
	-I$(top_srcdir)/src/sc2code/libs

noinst_LTLIBRARIES = libcomm.la
libcomm_la_LIBADD = $(patsubst %.c, %.lo, $(wildcard */*.c))
libcomm_la_SOURCES = dummy.c

%.lo: %.c
	$(COMPILE) -c $< -o $@

clean: clean-am
	rm -f */*.{o,lo}

distclean: distclean-am
	rm -f */*.{o,lo}

