# autoconf master file. Mudry <mudry@users.sf.net>
# Read README.ac!

# Process this file with autoconf to produce a configure script.

# Initialize autoconf
AC_PREREQ(2.50)
AC_INIT(src/starcon2.c)

# Detect the canonical host and target build environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Define package name and version here. This should be the only place
# where the actual version number appears! Use the PACKAGE and VERSION
# varaibles/defines elsewhere.
AM_INIT_AUTOMAKE(sc2, 0.1)

# Make current date available for our tools
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

# This allows "--enable-maintainer-mode"
AM_MAINTAINER_MODE

# --------------------------------------------------------------------------

# Configurables

CFLAGS="$CFLAGS -g -W -Wall -DHAVE_OPENGL -DSDL_PTHREADS -DGFXMODULE_SDL -DSOUNDMODULE_SDL"
CPPFLAGS="$CPPFLAGS -g -W -Wall -DHAVE_OPENGL -DSDL_PTHREADS -DGFXMODULE_SDL -DSOUNDMODULE_SDL"

# --------------------------------------------------------------------------

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for typedefs, structures, and compiler characteristics.
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
#AC_CHECK_TYPES(int8_t)
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_EXEEXT

# Checks for library functions.
# AC_FUNC_ALLOCA
# AC_FUNC_MALLOC
AC_CHECK_FUNCS([strupr])

# --------------------------------------------------------------------------

# This section checks for special libs, as SDL and OpenGL

# Checks for misc libs
#AC_CHECK_LIB([m], [sin])
#AC_CHECK_LIB([ltdl], [lt_dlopen])
#AC_CHECK_LIB([SDL_image],[IMG_Load])
#AC_CHECK_LIB([SGE],[sge_tt_textout])

# Check for SDL
SDL_VERSION=1.2.2
AM_PATH_SDL($SDL_VERSION, , AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
AC_CHECK_LIB(SDL_mixer, main, , AC_MSG_ERROR([Your system is missing libSDL_mixer!]),`sdl-config --libs`)
AC_CHECK_LIB(SDL_image, main, , AC_MSG_ERROR([Your system is missing libSDL_image!]),`sdl-config --libs`)
AC_CHECK_LIB(SDL_sound, main, , AC_MSG_ERROR([Your system is missing libSDL_sound!]),`sdl-config --libs`)
#AC_CHECK_HEADERS(SDL/SDL_image.h, , AC_MSG_ERROR([Your system is missing SDL_image.h!]))
#AC_CHECK_HEADERS(SDL/SDL_mixer.h, , AC_MSG_ERROR([Your system is missing SDL_mixer.h!]))

# Select platform dependent OpenGL libs.. (tweak your platform here)
case "$target" in
	*-*-linux-gnu*)
		SYS_GL_LIBS="-lGL"
		;;
	*-*-cygwin* | *-*-mingw32*)
		SYS_GL_LIBS="-lopengl32"
		SYS_GL_CFLAGS="-DGL_VERSION_1_2"
		;;
	*-*-beos*)
		SYS_GL_LIBS="-lGL"
		;;
	*-*-darwin*)
		SYS_GL_LIBS=""
		;;
	*-*-freebsd*)
		AC_PATH_X
		if test x$no_x != xyes; then
			CFLAGS="$CFLAGS -L $x_libraries"
		fi
		SYS_GL_LIBS="-lGL"
		;;
	*)
		if test x$have_x = xyes; then
			CFLAGS="$CFLAGS $X_CFLAGS"
			SYS_GL_LIBS="$X_LIBS -lGL"
		else
			SYS_GL_LIBS="-lGL"
		fi
		;;
esac

# Check for OpenGL
AC_MSG_CHECKING(for OpenGL support)
have_opengl=no
AC_TRY_COMPILE([
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
],[
],[
	have_opengl=yes
])
AC_MSG_RESULT($have_opengl)
GL_LIBS=$SYS_GL_LIBS
AC_SUBST(GL_LIBS)

# Figure out which math library to use
case "$target" in
	*-*-linux-gnu*)
		MATHLIB=""
		;;
	*-*-cygwin* | *-*-mingw32*)
		MATHLIB=""
		;;
	*-*-beos*)
		MATHLIB=""
		;;
	*-*-darwin*)
		MATHLIB=""
		;;
	*)
		MATHLIB="-lm"
		AC_PATH_X
		AC_PATH_XTRA
		if test x$have_x = xyes; then
			CFLAGS="$CFLAGS $X_CFLAGS"
		fi
		;;
esac
AC_SUBST(MATHLIB)

# --------------------------------------------------------------------------

# This section is optional but contains suggested features such as libtool 
# and gettext support.

# Libtool support
AM_DISABLE_SHARED
AM_PROG_LIBTOOL

# Use this if we going to use dlopen (dynamic modules)
# LIBADD_DL=-dlopen

# Internationalization support
# AM_GNU_GETTEXT

# supported languages:
# ALL_LINGUAS=""

# Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
	AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/${DATADIRNAME}/locale$", "Locale files")
else
	AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale", "Locale files")
fi

# --------------------------------------------------------------------------
# User selectable features (conditionals)

AC_ARG_ENABLE(debug,
	[  --enable-debug          Turn on debugging],
	[case "${enableval}" in
	 yes) debug=true ; AC_DEFINE(DEBUG,,Include debugging code) ;;
	 no)  debug=false ;;
	 *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
	esac],
	[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)
if test "x$debug" = "xtrue"; then
	CFLAGS="$CFLAGS -g"
fi

# --------------------------------------------------------------------------

# Output configuration

AM_CONFIG_HEADER(config.h)

# "intl/Makefile po/Makefile.in m4/Makefile" is added when
# autogen.sh runs. 

AC_CONFIG_FILES([
	Makefile
	src/Makefile 
	src/sc2code/Makefile
	src/sc2code/libs/Makefile
	src/sc2code/libs/decomp/Makefile
	src/sc2code/libs/memory/Makefile
	src/sc2code/libs/strings/Makefile
	src/sc2code/libs/resource/Makefile
	src/sc2code/libs/math/Makefile
	src/sc2code/libs/input/Makefile
	src/sc2code/libs/input/sdl/Makefile
	src/sc2code/libs/sound/Makefile
	src/sc2code/libs/sound/sdl/Makefile
	src/sc2code/libs/graphics/Makefile
	src/sc2code/libs/graphics/sdl/Makefile
	src/sc2code/libs/task/Makefile
	src/sc2code/libs/threads/Makefile
	src/sc2code/libs/threads/sdl/Makefile
	src/sc2code/libs/time/Makefile
	src/sc2code/libs/time/sdl/Makefile
	src/sc2code/libs/video/Makefile
	src/sc2code/planets/Makefile
	src/sc2code/ships/Makefile
	src/sc2code/comm/Makefile
])

AC_OUTPUT

