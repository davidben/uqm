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

char *strupr (char *str);

#endif  /* _PORT_H */

