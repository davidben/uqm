#!/bin/sh
find . -name "*.o" -print0 | xargs -0 rm -f
find . -name "*.a" -print0 | xargs -0 rm -f
find . -name "*.lo" -print0 | xargs -0 rm -f
find . -name "*.la" -print0 | xargs -0 rm -f
find . -name .libs -print0 | xargs -0 rm -rf
find . -name .deps -print0 | xargs -0 rm -rf
find . -name Makefile -print0 | xargs -0 rm -rf
find . -name Makefile.in -print0 | xargs -0 rm -rf
rm -f aclocal.m4 config.guess config.h config.h.in config.log config.status \
		config.sub configure \
		install-sh libtool ltmain.sh missing mkinstalldirs \
		stamp-h stamp-h.in src/sc2
rm -rf libltdl
rm -rf autom4te.cache
