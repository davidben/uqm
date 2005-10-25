/* This file contains some compile-time configuration options.
 */

#ifdef _MSC_VER
	/* In this case, build.sh is not run to generate a config file, so
	 * we use a default file msvc++/config.h instead.
	 * If you want anything else than the defaults, you'll have to edit
	 * that file manually. */
#	include "msvc++/config.h"
#elif defined (__MINGW32__) || defined (__CYGWIN__)
	/* If we're compiling on MS Windows using build.sh, use
	 * config_win.h, generated from * src/config_win.h.in. */
#	include "config_win.h"
#else
	/* If we're compiling in unix, use config_unix.h, generated from
	 * src/config_unix.h.in by build.sh. */
#	include "config_unix.h"
#endif

