#! /bin/sh -e

if [ "$#" != 0 ]; then
  case "$1" in
    (--version)
      cat <<EOF
autogen.sh 
CVS $Id$
Written by Mudry <mudry@users.sf.net> and Kalle Olavi Niemitalo
(originally for the FSCP)
This program is free software; you may redistribute it under the terms of
the GNU General Public License.  This program has absolutely no warranty.
EOF
      exit 0
      ;;
    (--help)
      cat <<EOF
Usage: $0 [OPTION]
Generate files which are in distribution tarball but not in CVS.

  --help     Display this help and exit.
  --version  Display version information and exit.

Report bugs to <sc2-devel@lists.sourceforge.net>.
EOF
      exit 0
      ;;
    (-*)
      # Don't parse backslashes in $0 or $1.
      printf >&2 "%s: invalid option -- %s\n" "$0" "$1"
      printf >&2 "Try \"%s --help\" for more information.\n" "$0"
      exit 1
      ;;
    (*)
      printf >&2 "%s: non-option arguments don't work.\n"
      printf >&2 "Try \"%s --help\" for more information.\n" "$0"
      exit 1
      ;;
  esac
fi

# Like printf, except prepend the program name and append a newline.
# This is used for error messages too, so don't check --quiet here.
note ()
{
  local fmt
  fmt="$1"
  shift
  printf "%s: ${fmt}\n" "$0" "$@"
}

if [ ! -d libltdl ]; then
  note "Running libtoolize"
  libtoolize --automake --ltdl
fi

#if [ ! -d po ]; then
#  note "Running gettextize"
#  gettextize --intl
#fi

#if [ ! -f po/POTFILES.in ]; then
#  note "Making POTFILES.in"
#  ls src/*.[ch] src/sc2code/*.[ch] src/sc2code/*/*.[ch] > po/POTFILES.in
#fi 

if [ ! -f aclocal.m4 ]; then
  note "Running aclocal"
  aclocal
fi

if [ ! -x config.h.in ]; then
  note "Running autoheader"
  autoheader
fi

if [ ! -x configure ]; then
  note "Running autoconf"
  autoconf
fi

if [ ! -f ChangeLog ]; then
  if [ -x /usr/bin/cvs2cl ]; then
    note "Creating ChangeLog from CVS"
    cvs2cl -P -r -t -U AUTHORS
    # If there's no route to the CVS server, cvs2cl exits with code 0
    # but does not create ChangeLog.  We notice that below.
  fi
  if [ ! -f ChangeLog ]; then
    note "Creating dummy ChangeLog"
    touch ChangeLog
  fi
fi

if [ ! -e INSTALL.generic ]; then
  # There is aclocal --print-ac-dir, but no such thing for Autoconf.  :-(
  # We can't grep autoconf_dir "$(which autoconf)" either, because
  # Debian puts the real Autoconf in /usr/bin/autoconf2.50 and makes
  # /usr/bin/autoconf a wrapper.
  found=
  for dir in /usr/local/share/autoconf /usr/share/autoconf; do
    if [ -e "${dir}/INSTALL" ]; then
      note "Linking INSTALL.generic"
      ln -s "${dir}/INSTALL" INSTALL.generic
      found=t
      break
    fi
  done
  if [ -z "${found}" ]; then
    note >&2 "Autoconf directory not found"
    exit 1
  fi
fi

note "Running automake"
automake --add-missing
