/* This file contains some compile-time configuration options.
 */

#ifdef WIN32
	/* If we're compiling in windows, we're using the msvc++/config.h file
	 * If you want anything else than the defaults, you'll have to edit
	 * that file manually. */
#	include "msvc++/config.h"
#else
	/* If we're compiling in unix, use config_unix.h, generated from
	 * src/config_unix.h.in by build.sh. */
#	include "config_unix.h"
#endif

