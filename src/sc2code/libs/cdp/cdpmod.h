/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * CDP module definitions
 * all CDP modules should #include this .h
 */

#ifndef _CDPMOD_H
#define _CDPMOD_H

#include "types.h"
#include "cdpapi.h"

#define CDP_INFO_SYM      cdpmodinfo
#define CDP_INFO_SYM_NAME "cdpmodinfo"

// this struct will be exported from the module
// under 'cdpmodinfo'
typedef struct
{
	// mandatory, version control
	uint32 size;             // size of this structure
	cdp_ApiVersion api_ver;  // version of cdp API used, set to CDPAPI_VERSION
	uint16 ver_major;        // module version, somewhat informational
	uint16 ver_minor;
	uint16 host_ver_major;   // minimum host version required, purely informational
	uint16 host_ver_minor;
	
	uint32 _32_reserved1[4]; // reserved members: set to all 0s or CDP_MODINFO_RESERVED1

	// informational, human-only
	const char* name;
	const char* ver_string;
	const char* author;
	const char* url;
	const char* comments;
	
	const char* _sz_reserved2[5]; // reserved members, set to all 0s or CDP_MODINFO_RESERVED2

	// mandatory, CDP entry points
	// TODO: decide whether more EPs are necessary and if not move
	// EPs above info-string members, abolishing _sz_reserved2
	bool (* module_init) (cdp_Itf_Host* hostitf);
	void (* module_term) ();
	
} cdp_ModuleInfo;

#define CDP_MODINFO_RESERVED1  {0,0,0,0}
#define CDP_MODINFO_RESERVED2  {0,0,0,0,0}

// the following is defined via the last mandatory member
#define CDP_MODINFO_MIN_SIZE \
	( ((uint32) &((cdp_ModuleInfo*)0)->module_term) + \
	sizeof (((cdp_ModuleInfo*)0)->module_term) )

#if defined(WIN32)
#	define CDPEXPORT __declspec(dllexport)
#else
#	define CDPEXPORT
#endif

#endif  /* _CDPMOD_H */
