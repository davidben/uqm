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
 * CDP API definitions
 * the API is used by both the engine and modules
 */

#include "cdp.h"
#include "port.h"
#include "cdpint.h"
#include "uqmversion.h"

#define MAX_REG_ITFS 255

static cdp_Error cdp_api_error = CDPERR_NONE;

static uint32 cdp_Host_GetApiVersion (void);
static uint32 cdp_Host_GetVersion (void);
static cdp_Error cdp_Host_GetApiError (void);
static cdp_Itf* cdp_Host_GetItf (cdp_ItfKind);
static cdp_RegItf* cdp_Host_RegisterItf (cdp_ItfKind,
			cdp_ApiVersion ver_from, cdp_ApiVersion ver_to,
			cdp_Itf*, cdp_Module*);
static void cdp_Host_UnregisterItf (cdp_RegItf*);

// Interfaces
cdp_Itf_HostVtbl_v1 cdp_host_itf_v1 = 
{
	cdp_Host_GetApiVersion,
	cdp_Host_GetVersion,
	cdp_Host_GetApiError,
	cdp_Host_GetItf,
	cdp_Host_RegisterItf,
	cdp_Host_UnregisterItf,
};

cdp_Itf_MemoryVtbl_v1 cdp_memory_itf_v1 =
{
	HMalloc,
	HFree,
	HCalloc,
	HRealloc,
};

cdp_Itf_IoVtbl_v1 cdp_io_itf_v1 = 
{
	uio_fopen,
	uio_fclose,
	uio_fread,
	uio_fwrite,
	uio_fseek,
	uio_ftell,
	uio_fflush,
	uio_feof,
	uio_ferror,
};

cdp_Itf_SoundVtbl_v1 cdp_sound_itf_v1 = 
{
	SoundDecoder_Register,
	SoundDecoder_Unregister,
	SoundDecoder_Lookup,
};

cdp_Itf_VideoVtbl_v1 cdp_video_itf_v1 = 
{
	VideoDecoder_Register,
	VideoDecoder_Unregister,
	VideoDecoder_Lookup,
};

struct cdp_RegItf
{
	bool builtin;
	bool used;
	cdp_ItfKind kind;
	cdp_ApiVersion ver_from;
	cdp_ApiVersion ver_to;
	cdp_Itf* itfvtbl;
	cdp_Module* module;
};

#define CDP_DECLARE_ITF(k,vf,vt,vtbl) \
	{true, true, CDPITF_KIND_##k, \
	CDPAPI_VERSION_##vf, CDPAPI_VERSION_##vt, vtbl, NULL}

// Built-in interfaces + space for loadable
cdp_RegItf cdp_itfs[MAX_REG_ITFS + 1] =
{
	CDP_DECLARE_ITF (HOST,   1, 1, &cdp_host_itf_v1),
	CDP_DECLARE_ITF (MEMORY, 1, 1, &cdp_memory_itf_v1),
	CDP_DECLARE_ITF (IO,     1, 1, &cdp_io_itf_v1),
	CDP_DECLARE_ITF (SOUND,  1, 1, &cdp_sound_itf_v1),
	CDP_DECLARE_ITF (VIDEO,  1, 1, &cdp_video_itf_v1),
	// TODO: put newly defined built-in interfaces here

	{false, false, CDPITF_KIND_INVALID, 0, 0, NULL} // term
};

cdp_Error
cdp_GetApiError (void)
{
	cdp_Error ret = cdp_api_error;
	cdp_api_error = CDPERR_NONE;
	return ret;
}

bool
cdp_InitApi (void)
{
	// TODO: add whatever init code is necessary
	return true;
}

void
cdp_UninitApi (void)
{
	cdp_RegItf* itf;

	// unregister custom interfaces
	for (itf = cdp_itfs; itf->used; ++itf)
	{
		if (!itf->builtin)
		{
			itf->used = false;
			itf->kind = CDPITF_KIND_INVALID;
			itf->itfvtbl = NULL;
		}
	}
}

cdp_Itf*
cdp_GetInterface (cdp_ItfKind itf_kind, cdp_ApiVersion api_ver)
{
	cdp_RegItf* itf;

	for (itf = cdp_itfs; itf->used &&
			(itf->kind != itf_kind ||
			 api_ver < itf->ver_from || api_ver > itf->ver_to);
			itf++)
		;
	if (!itf->kind)
	{
		cdp_api_error = CDPERR_NO_ITF;
		return NULL;
	}
	
	return itf->itfvtbl;
}

static uint32
cdp_Host_GetApiVersion (void)
{
	return CDPAPI_VERSION;
}

static uint32
cdp_Host_GetVersion (void)
{
	return (UQM_MAJOR_VERSION << 16) | UQM_MINOR_VERSION;
}

static cdp_Error
cdp_Host_GetApiError (void)
{
	return cdp_GetApiError ();
}

static cdp_Itf*
cdp_Host_GetItf (cdp_ItfKind itf_kind)
{
	return cdp_GetInterface (itf_kind, CDPAPI_VERSION_1);
}

static cdp_RegItf*
cdp_Host_RegisterItf (cdp_ItfKind itf_kind, cdp_ApiVersion ver_from,
		cdp_ApiVersion ver_to,	cdp_Itf* itfvtbl, cdp_Module* module)
{
	cdp_RegItf* regitf;
	cdp_RegItf* newslot = NULL;

	if (!module)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"No module info supplied\n");
		//return NULL;
	}
	if (!itf_kind || !itfvtbl)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Null or invalid interface (from %s)\n",
				cdp_GetModuleName (module));
		return NULL;
	}
	// TODO: review version policy (below)
	// enforce version policy and do not allow obsolete interfaces
	// POLICY: all modules MUST be aware of recent API changes and will not
	// be allowed to expose interfaces that support and/or utilize obsoleted
	// API versions
	if (ver_from < CDPAPI_VERSION_MIN)
		ver_from = CDPAPI_VERSION_MIN;
	if (ver_to < CDPAPI_VERSION_MIN)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Obsolete interface 0x%08x (from %s)\n",
				itf_kind, cdp_GetModuleName (module));
		return NULL;
	}

	// check if extension already registered
	for (regitf = cdp_itfs; regitf->used &&
			(regitf->kind != itf_kind ||
			 ver_from < regitf->ver_from || ver_to > regitf->ver_to);
			++regitf)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !regitf->kind)
			newslot = regitf;
	}

	if (regitf >= cdp_itfs + MAX_REG_ITFS)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Interfaces limit reached\n");
		return NULL;
	}
	else if (regitf->kind)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"interface 0x%08x already registered for these versions, "
				"%s denied\n",
				itf_kind, cdp_GetModuleName (module));
		return NULL;
	}
	
	if (!newslot)
	{
		newslot = regitf;
		newslot->used = true;
		// make next one a term
		regitf[1].builtin = false;
		regitf[1].used = false;
		regitf[1].kind = CDPITF_KIND_INVALID;
		regitf[1].itfvtbl = NULL;
	}

	newslot->kind = itf_kind;
	newslot->ver_from = ver_from;
	newslot->ver_to = ver_to;
	newslot->itfvtbl = itfvtbl;
	newslot->module = module;
	
	return newslot;
}

static void
cdp_Host_UnregisterItf (cdp_RegItf* regitf)
{
	if (regitf < cdp_itfs || regitf >= cdp_itfs + MAX_REG_ITFS ||
			!regitf->kind || !regitf->itfvtbl)
	{
		fprintf (stderr, "cdp_Host_UnregisterItf(): "
				"Invalid or expired interface passed\n");
		return;
	}
	
	regitf->kind = CDPITF_KIND_INVALID;
	regitf->itfvtbl = NULL;
}
