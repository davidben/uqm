#ifndef _PORT_H
#define _PORT_H

#include "config.h"

#ifdef WIN32
#define alloca _alloca
#define snprintf _snprintf
#endif

#ifndef HAVE_STRICMP
#define stricmp strcasecmp
#endif

#ifndef HAVE_STRUPR
char *strupr (char *str);
#endif

#endif  /* _PORT_H */

