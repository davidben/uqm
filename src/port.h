#ifndef _PORT_H
#define _PORT_H

#include "config.h"

#ifdef _MSC_VER
#define alloca _alloca
#define snprintf _snprintf
#endif

#ifndef HAVE_STRICMP
#define stricmp strcasecmp
#endif

#ifndef HAVE_STRUPR
char *strupr (char *str);
#endif

#ifdef WIN32
#	include <stdlib.h>
#	define PATH_MAX  _MAX_PATH
#	define NAME_MAX  _MAX_FNAME
		// _MAX_DIR and FILENAME_MAX could also be candidates.
		// If anyone can tell me which one matches NAME_MAX, please
		// tell me. - SvdB
#else
	/* PATH_MAX is per POSIX defined in <limits.h> */
#	include <limits.h>
#endif
#ifdef _MSC_VER
#	include <io.h>
typedef int ssize_t;
typedef unsigned short mode_t;
#	define access _access
#	define F_OK 0
#	define W_OK 2
#	define R_OK 4
#	include <direct.h>
#	define open _open
#	define mkdir _mkdir
#	define read _read
#	define rmdir _rmdir
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
typedef int ssize_t;
typedef unsigned short mode_t;
#	define S_IRWXU (S_IREAD | S_IWRITE | S_IEXEC)
#	define S_IRWXG 0
#	define S_IRWXO 0
#	define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#	define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#else
#	include <unistd.h>
#endif

#endif  /* _PORT_H */

