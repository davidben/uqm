#ifdef WIN32

/* To avoid including windows.h,
   Win32's <GL/gl.h> needs APIENTRY and WINGDIAPI defined properly. */

#pragma warning (disable:4244)	/* Disable bogus conversion warnings. */

#ifndef APIENTRY
#define GLUT_APIENTRY_DEFINED
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define APIENTRY    __stdcall
#else
#define APIENTRY
#endif
#endif

/* This is from Win32's <winnt.h> */
#ifndef CALLBACK
#if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#define CALLBACK __stdcall
#else
#define CALLBACK
#endif
#endif

/* This is from Win32's <wingdi.h> and <winnt.h> */
#ifndef WINGDIAPI
#define GLUT_WINGDIAPI_DEFINED
#define WINGDIAPI __declspec(dllimport)
#endif

/* This is from Win32's <ctype.h> */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#include "GL/glu.h"

#else /* !defined(WIN32) */

#include "GL/glu.h"

#endif

