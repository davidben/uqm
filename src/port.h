#ifdef WIN32
#define HAVE_STRICMP
#define HAVE_STRUPR
#endif

#ifndef HAVE_STRICMP
#define stricmp strcasecmp
#endif

#ifndef HAVE_STRUPR
char *strupr (char *str);
#endif

