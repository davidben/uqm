#!/bin/sh
# Script for creating self-extracting installer files for unix-like systems.
# By Serge van den Boom, 2003-02-20

TEMPDIR="/tmp/buildinstaller_$$"

if [ $# -ne 2 ]; then
	cat >&2 << EOF
Usage: buildinstaller.sh <installername> <contentfiles>
where 'installername' is the name you want to give the final installer
and 'contentfiles' is a space-seperated list of possible content files.
EOF
	exit 1
fi
DESTFILE="$1"
CONTENTFILES="$2"

if [ ! -d src -o ! -d build ]; then
cat >&2 << EOF
Error: The current directory should be the top of the cvs tree.
       Please try again from that dir
EOF
	exit 1
fi

if [ ! -f uqm ]; then
	cat >&2 << EOF
Error: There should be an 'uqm' binary in the top of the cvs tree.
       please recompile and try again.
EOF
	exit 1
fi

mkdir "$TEMPDIR" 2> /dev/null

if [ "$?" -ne 0 ]; then
	echo "Could not create temporary dir." >&2
	exit 1
fi

mkdir -- "${TEMPDIR}/lib" "${TEMPDIR}/lib/uqm" "${TEMPDIR}/lib/uqm/doc"

cp -- AUTHORS COPYING ChangeLog WhatsNew README doc/users/manual.txt \
		"${TEMPDIR}/lib/uqm/doc/"
cp -- uqm "${TEMPDIR}/lib/uqm/"
chmod -R go+rX "${TEMPDIR}/lib"

echo "Making tar.gz file for everything except from the content."
echo "Using maximum compression; this may take a moment."
tar -c -C "${TEMPDIR}/lib/" uqm | gzip -9 > "${TEMPDIR}/libpkg.tar.gz"

echo "Making final executable."
sh build/unix_installer/mkinstall "$DESTFILE" "${TEMPDIR}/libpkg.tar.gz" \
		"$CONTENTFILES" COPYING

rm -r -- "$TEMPDIR"

chmod 755 "$DESTFILE"


