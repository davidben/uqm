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
 * CDP API declarations
 * the API is used by both the engine and modules
 */

#ifndef _CDPAPI_H
#define _CDPAPI_H

#include "types.h"

typedef enum
{
	CDPAPI_VERSION_1 = 0x00000001, // version 0.1
	
	CDPAPI_VERSION     = CDPAPI_VERSION_1,
	CDPAPI_VERSION_MIN = CDPAPI_VERSION_1,

} cdp_ApiVersion;

typedef enum
{
	CDPERR_NONE        = 0,
	CDPERR_UKNOWN      = 1,
	CDPERR_NOT_FOUND   = 2,
	CDPERR_BAD_MODULE  = 3,
	CDPERR_OLD_VER     = 4,
	CDPERR_UNKNOWN_VER = 5,
	CDPERR_TOO_MANY    = 6,
	CDPERR_INIT_FAILED = 7,
	CDPERR_NO_ITF      = 8,
	CDPERR_DUPE_ITF    = 9,
	CDPERR_OTHER       = 1000,

} cdp_Error;

typedef struct cdp_Module cdp_Module;
typedef void cdp_Itf;
typedef struct cdp_RegItf cdp_RegItf;

typedef enum
{
	CDPITF_KIND_INVALID  = 0,
	CDPITF_KIND_HOST     = 1,
	CDPITF_KIND_MEMORY   = 2,
	CDPITF_KIND_IO       = 3,
	CDPITF_KIND_THREADS  = 4,
	CDPITF_KIND_TIME     = 5,
	CDPITF_KIND_INPUT    = 6,
	CDPITF_KIND_TASK     = 7,
	CDPITF_KIND_RESOURCE = 8,
	CDPITF_KIND_SOUND    = 9,
	CDPITF_KIND_VIDEO    = 10,
	CDPITF_KIND_GFX      = 11,
	CDPITF_KIND_MIXER    = 12,
	// anything up to 65536 is reserved
	// apply for your KIND (range) officially in UQM bugzilla (for now)

	// custom interfaces live here
#	include "cdpitfs.h"

} cdp_ItfKind;

// Host Interface
// the main itf of the API, it is passed to a loaded module
// module does everything else through this itf and itfs
// acquired through this itf
typedef struct
{
	uint32 (* GetApiVersion) (void);
	uint32 (* GetVersion) (void);
	cdp_Error (* GetApiError) (void);
	cdp_Itf* (* GetItf) (cdp_ItfKind);
	cdp_RegItf* (* RegisterItf) (cdp_ItfKind,
			cdp_ApiVersion ver_from, cdp_ApiVersion ver_to,
			cdp_Itf*, cdp_Module*);
	void (* UnregisterItf) (cdp_RegItf*);

} cdp_Itf_HostVtbl_v1;

typedef cdp_Itf_HostVtbl_v1 cdp_Itf_HostVtbl;
typedef cdp_Itf_HostVtbl    cdp_Itf_Host;

#endif  /* _CDPAPI_H */
