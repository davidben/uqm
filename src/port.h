#ifndef _PORT_H
#define _PORT_H

#include "config.h"

#ifdef WIN32
#	include <io.h>
#else
#	include <unistd.h>
#endif

#ifndef HAVE_STRICMP
#	define stricmp strcasecmp
#else
#	define strcasecmp stricmp
#endif

#ifndef HAVE_STRUPR
char *strupr (char *str);
#endif


// Compilation related
#ifdef _MSC_VER
#	define inline __inline
#else
#	define inline __inline__
#endif

// Directories
#ifdef WIN32
#	include <stdlib.h>
#	define PATH_MAX  _MAX_PATH
#	define NAME_MAX  _MAX_FNAME
		// _MAX_DIR and FILENAME_MAX could also be candidates.
		// If anyone can tell me which one matches NAME_MAX, please
		// let me know. - SvdB
#else
#	include <limits.h>
		/* PATH_MAX is per POSIX defined in <limits.h>, but:
		 * "A definition of one of the values from Table 2.6 shall bea
		 * omitted from <limits.h> on specific implementations where the
		 * corresponding value is equal to or greater than the
		 * stated minimum, but where the value can vary depending
		 * on the file to which it is applied. The actual value supported
		 * for a specific pathname shall be provided by the pathconf()
		 * function."
		 * _POSIX_NAME_MAX will provide a minimum (14).
		 * This is relevant (at least) for Solaris.
		 */
#	ifndef NAME_MAX
#		define NAME_MAX _POSIX_NAME_MAX
#	endif
#endif

// Some types
#ifdef WIN32
typedef int ssize_t;
#endif
#ifdef _MSC_VER
typedef unsigned short mode_t;
#endif

// Directories
#include <sys/stat.h>
#ifdef WIN32
#	ifdef _MSC_VER
#		define MKDIR(name, mode) ((void) mode, _mkdir(name))
#	else
#		define MKDIR(name, mode) ((void) mode, mkdir(name))
#	endif
#else
#	define MKDIR mkdir
#endif
#ifdef _MSC_VER
#	include <direct.h>
#	define chdir _chdir
#	define getcwd _getcwd
#	define access _access
#	define F_OK 0
#	define W_OK 2
#	define R_OK 4
#	define open _open
#	define read _read
//#	define fstat _fstat
#	define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#	define S_IRWXU (S_IREAD | S_IWRITE | S_IEXEC)
#	define S_IRWXG 0
#	define S_IRWXO 0
#	define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#	define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#	define write _write
//#	define stat _stat
#	define unlink _unlink
#elif defined (__MINGW32__)
#	define S_IRWXG 0
#	define S_IRWXO 0
#endif

// Memory
#ifdef WIN32
#	ifdef __MINGW32__
#		include <malloc.h>
#	elif defined (_MSC_VER)
#		define alloca _alloca
#	endif
#elif defined(__linux__) || defined(__svr4__)
#	include <alloca.h>
#endif

// Printf
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

// setenv()
#ifndef HAVE_SETENV
int setenv (const char *name, const char *value, int overwrite);
#endif

#ifndef HAVE_WCHAR_T
typedef unsigned short wchar_t;
#endif

#ifndef HAVE_WINT_T
typedef unsigned int wint_t;
#endif

#ifdef HAVE_ISWGRAPH
#	include <wctype.h>
#else
#	include <ctype.h>
static inline int
iswgraph(wint_t wc)
{
	return wc > 0x7f || isgraph((int) wc);
}
#endif

// Use SDL_INCLUDE and SDL_IMAGE_INCLUDE to portably include the SDL files
// from the right location.
// TODO: Where the SDL and SDL_image headers are located could be detected
//       from the build script.
#ifdef __APPLE__
	// SDL_image.h in a directory SDL_image under the include dir.
#	define SDL_DIR SDL
#	define SDL_IMAGE_DIR SDL_image
#else
	// SDL_image.h directly under the include dir.
#	undef SDL_DIR
#	undef SDL_IMAGE_DIR
#endif

#ifdef SDL_DIR
#	define SDL_INCLUDE(file) <SDL_DIR/file>
#else
#	define SDL_INCLUDE(file) <file>
#endif  /* SDL_DIR */

#ifdef SDL_IMAGE_DIR
#	define SDL_IMAGE_INCLUDE(file) <SDL_IMAGE_DIR/file>
#else
#	define SDL_IMAGE_INCLUDE(file) <file>
#endif  /* SDL_IMAGE_DIR */

#endif  /* _PORT_H */

