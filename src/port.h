#ifndef _PORT_H
#define _PORT_H

#include "config.h"

#ifdef __MINGW32__
// Microsoft Windows headers expect this to be set. The MSVC compiler sets
// it, but MinGW doesn't.
#	if defined(_X86_)
#		define _M_IX86
#	elif defined(_IA64_)
#		define _M_IA64
#	elif defined(__amd64__)
#		define _M_AMD64
#	elif defined(__m68k__)
#		define _68K_
#	elif defined(__ppc__)
#		define _M_PPC
#	endif
#endif


// Compilation related
#ifdef _MSC_VER
#	define inline __inline
#else
#	define inline __inline__
#	ifdef __MINGW32__
		// For when including Microsoft Windows header files.
#		define _inline inline
#	endif
#endif

// Compilation warnings:
// UQM uses a lot of functions that can be used unsafely, but it uses them
// in a safe way. The warnings about these functions however may drown out
// serious warnings, so we turn them off.
#ifdef _MSC_VER
#	define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef _MSC_VER
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

#if !defined (_MSC_VER) && !defined (HAVE_READDIR_R)
#	include <dirent.h>
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
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
#ifdef _MSC_VER
typedef int ssize_t;
typedef unsigned short mode_t;
#endif

// Directories
#include <sys/stat.h>
#ifdef _MSC_VER
#	define MKDIR(name, mode) ((void) mode, _mkdir(name))
#elif defined(__MINGW32__)
#	define MKDIR(name, mode) ((void) mode, mkdir(name))
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
#define vsnprintf _vsnprintf
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

#if defined (_MSC_VER) || defined(__MINGW32__)
#	define USE_WINSOCK
#endif

// errno error numbers. The values used don't matter, as long as they
// don't conflict with existing errno error numbers.
#ifdef PORT_WANT_ERRNO
#	ifdef USE_WINSOCK
#		include <errno.h>
#		ifndef E2BIG
#			define E2BIG            0x01000001
#		endif
#		ifndef EACCES
#			define EACCES           0x01000002
#		endif
#		ifndef EADDRINUSE
#			define EADDRINUSE       0x01000003
#		endif
#		ifndef EADDRNOTAVAIL
#			define EADDRNOTAVAIL    0x01000004
#		endif
#		ifndef EAFNOSUPPORT
#			define EAFNOSUPPORT     0x01000005
#		endif
#		ifndef EAGAIN
#			ifdef EWOULDBLOCK
#				define EAGAIN       EWOULDBLOCK
#			else
#				define EAGAIN       0x01000006
#			endif
#		endif
#		ifndef EALREADY
#			define EALREADY         0x01000007
#		endif
#		ifndef EBADF
#			define EBADF            0x01000008
#		endif
#		ifndef EBADMSG
#			define EBADMSG          0x01000009
#		endif
#		ifndef EBUSY
#			define EBUSY            0x0100000a
#		endif
#		ifndef ECANCELED
#			define ECANCELED        0x0100000b
#		endif
#		ifndef ECHILD
#			define ECHILD           0x0100000c
#		endif
#		ifndef ECONNABORTED
#			define ECONNABORTED     0x0100000d
#		endif
#		ifndef ECONNREFUSED
#			define ECONNREFUSED     0x0100000e
#		endif
#		ifndef ECONNRESET
#			define ECONNRESET       0x0100000f
#		endif
#		ifndef EDEADLK
#			define EDEADLK          0x01000010
#		endif
#		ifndef EDESTADDRREQ
#			define EDESTADDRREQ     0x01000011
#		endif
#		ifndef EDOM
#			define EDOM             0x01000012
#		endif
// Reserved in POSIX
//#		ifndef //EDQUOT
//#			define //EDQUOT         0x01000013
//#		endif
#		ifndef EEXIST
#			define EEXIST           0x01000014
#		endif
#		ifndef EFAULT
#			define EFAULT           0x01000015
#		endif
#		ifndef EFBIG
#			define EFBIG            0x01000016
#		endif
#		ifndef EHOSTUNREACH
#			define EHOSTUNREACH     0x01000017
#		endif
#		ifndef EIDRM
#			define EIDRM            0x01000018
#		endif
#		ifndef EILSEQ
#			define EILSEQ           0x01000019
#		endif
#		ifndef EINPROGRESS
#			define EINPROGRESS      0x0100001a
#		endif
#		ifndef EINTR
#			define EINTR            0x0100001b
#		endif
#		ifndef EINVAL
#			define EINVAL           0x0100001c
#		endif
#		ifndef EIO
#			define EIO              0x0100001d
#		endif
#		ifndef EISCONN
#			define EISCONN          0x0100001e
#		endif
#		ifndef EISDIR
#			define EISDIR           0x0100001f
#		endif
#		ifndef ELOOP
#			define ELOOP            0x01000020
#		endif
#		ifndef EMFILE
#			define EMFILE           0x01000021
#		endif
#		ifndef EMLINK
#			define EMLINK           0x01000022
#		endif
#		ifndef EMSGSIZE
#			define EMSGSIZE         0x01000023
#		endif
// Reserved in POSIX
//#		ifndef //EMULTIHOP
//#			define //EMULTIHOP      0x01000024
//#		endif
#		ifndef ENAMETOOLONG
#			define ENAMETOOLONG     0x01000025
#		endif
#		ifndef ENETDOWN
#			define ENETDOWN         0x01000026
#		endif
#		ifndef ENETRESET
#			define ENETRESET        0x01000027
#		endif
#		ifndef ENETUNREACH
#			define ENETUNREACH      0x01000028
#		endif
#		ifndef ENFILE
#			define ENFILE           0x01000029
#		endif
#		ifndef ENOBUFS
#			define ENOBUFS          0x0100002a
#		endif
#		ifndef ENODATA
#			define ENODATA          0x0100002b
#		endif
#		ifndef ENODEV
#			define ENODEV           0x0100002c
#		endif
#		ifndef ENOENT
#			define ENOENT           0x0100002d
#		endif
#		ifndef ENOEXEC
#			define ENOEXEC          0x0100002e
#		endif
#		ifndef ENOLCK
#			define ENOLCK           0x0100002f
#		endif
// Reserved in POSIX
//#		ifndef ENOLINK
//#			define ENOLINK          0x01000030
//#		endif
#		ifndef ENOMEM
#			define ENOMEM           0x01000031
#		endif
#		ifndef ENOMSG
#			define ENOMSG           0x01000032
#		endif
#		ifndef ENOPROTOOPT
#			define ENOPROTOOPT      0x01000033
#		endif
#		ifndef ENOSPC
#			define ENOSPC           0x01000034
#		endif
#		ifndef ENOSR
#			define ENOSR            0x01000035
#		endif
#		ifndef ENOSTR
#			define ENOSTR           0x01000036
#		endif
#		ifndef ENOSYS
#			define ENOSYS           0x01000037
#		endif
#		ifndef ENOTCONN
#			define ENOTCONN         0x01000038
#		endif
#		ifndef ENOTDIR
#			define ENOTDIR          0x01000039
#		endif
#		ifndef ENOTEMPTY
#			define ENOTEMPTY        0x0100003a
#		endif
#		ifndef ENOTSOCK
#			define ENOTSOCK         0x0100003b
#		endif
#		ifndef ENOTSUP
#			define ENOTSUP          0x0100003c
#		endif
#		ifndef ENOTTY
#			define ENOTTY           0x0100003d
#		endif
#		ifndef ENXIO
#			define ENXIO            0x0100003e
#		endif
#		ifndef EOPNOTSUPP
#			define EOPNOTSUPP       0x0100003f
#		endif
#		ifndef EOVERFLOW
#			define EOVERFLOW        0x01000040
#		endif
#		ifndef EPERM
#			define EPERM            0x01000041
#		endif
#		ifndef EPIPE
#			define EPIPE            0x01000042
#		endif
#		ifndef EPROTO
#			define EPROTO           0x01000043
#		endif
#		ifndef EPROTONOSUPPORT
#			define EPROTONOSUPPORT  0x01000044
#		endif
#		ifndef EPROTOTYPE
#			define EPROTOTYPE       0x01000045
#		endif
#		ifndef ERANGE
#			define ERANGE           0x01000046
#		endif
#		ifndef EROFS
#			define EROFS            0x01000047
#		endif
#		ifndef ESPIPE
#			define ESPIPE           0x01000048
#		endif
#		ifndef ESRCH
#			define ESRCH            0x01000049
#		endif
// Reserved in POSIX
//#		ifndef //ESTALE
//#			define //ESTALE         0x0100004a
//#		endif
#		ifndef ETIME
#			define ETIME            0x0100004b
#		endif
#		ifndef ETIMEDOUT
#			define ETIMEDOUT        0x0100004c
#		endif
#		ifndef ETXTBSY
#			define ETXTBSY          0x0100004d
#		endif
#		ifndef EWOULDBLOCK
#			ifdef EAGAIN
#				define EWOULDBLOCK  EAGAIN
#			else
#				define EWOULDBLOCK  0x0100004e
#			endif
#		endif
#		ifndef EXDEV
#			define EXDEV            0x0100004f
#		endif

// Non-POSIX:
#		ifndef EHOSTDOWN
#			define EHOSTDOWN        0x01100001
#		endif
#		ifndef EPFNOSUPPORT
#			define EPFNOSUPPORT     0x01100002
#		endif
#		ifndef EPROCLIM
#			define EPROCLIM         0x01100003
#		endif
#		ifndef ESHUTDOWN
#			define ESHUTDOWN        0x01100004
#		endif
#		ifndef ESOCKTNOSUPPORT
#			define ESOCKTNOSUPPORT  0x01100005
#		endif
#	elif  defined (__FreeBSD__) || defined (__OpenBSD__)
#		ifndef EBADMSG
#			define EBADMSG EIO
#		endif
#	endif  /* defined (__FreeBSD__) || defined (__OpenBSD__) */
#endif  /* defined (PORT_WANT_ERRNO) */

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

// Mark a function as using printf-style function arguments, so that
// extra consistency checks can be made by the compiler.
// The first argument to PRINTF_FUNCTION and VPRINTF_FUNCTION is the
// index of the format string argument, the second is the index of
// the first argument which is specified in the format string (in the
// case of PRINTF_FUNCTION) or of the va_list argument (in the case of
// VPRINTF_FUNCTION).
#ifdef __GNUC__
#	define PRINTF_FUNCTION(formatArg, firstArg) \
			__attribute__((format(printf, formatArg, firstArg)))
#	define VPRINTF_FUNCTION(formatArg) \
			__attribute__((format(printf, formatArg, 0)))
#else
#	define PRINTF_FUNCTION(formatArg, firstArg)
#	define VPRINTF_FUNCTION(formatArg)
#endif

#if defined(__GNUC__)
#	define _NORETURN __attribute__((noreturn))
#else
#	define _NORETURN
#endif

#endif  /* _PORT_H */

