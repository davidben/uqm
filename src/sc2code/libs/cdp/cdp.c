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
 * CDP library definitions
 */

#include <string.h>
#include <stdio.h>
#include "cdp.h"
#include "port.h"
#include "cdpint.h"
#include "cdpmod.h"
#include "uio.h"
#ifdef WIN32
#	include "windl.h"
#else
#	include <dlfcn.h>
#endif

#define MAX_CDPS 63
#define CDPDIR "cdps"

// internal adl module representation
// cdp_Module will be cast to and from this
struct cdp_Module
{
	bool used;         // used at least once indicator
	void* hmodule;     // loaded module handle
	uint32 refcount;   // reference count
	cdp_ModuleInfo* info; // cdp exported info

};

extern uio_DirHandle *cdpDir;

static bool cdp_inited = false;
static cdp_Module cdp_modules[MAX_CDPS + 1];
static cdp_Error cdp_last_error = CDPERR_NONE;
static char cdp_path[PATH_MAX] = "";

cdp_Error
cdp_GetError (void)
{
	cdp_Error ret = cdp_last_error;
	cdp_last_error = CDPERR_NONE;
	return ret;
}

bool
cdp_Init (void)
{
	if (cdp_inited)
	{
		fprintf (stderr, "cdp_Init(): called when already inited\n");
		return true;
	}

	memset (cdp_modules, 0, sizeof cdp_modules);
	//strcpy (cdp_path, cdpDir->path);
	cdp_InitApi ();
	cdp_inited = true;

	return true;
}

void
cdp_Uninit (void)
{
	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_Uninit(): called when not inited\n");
		return;
	}
	
	cdp_UninitApi ();
	cdp_FreeAllModules ();
	cdp_inited = false;
}

cdp_Module*
cdp_LoadModule (const char* modname)
{
	void* mod;
	char modpath[PATH_MAX];
	const char* errstr;
	cdp_ModuleInfo* info;
	int i;
	cdp_Module* cdp;
	cdp_Module* newslot = 0;
	cdp_Itf* ihost;

	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_LoadModule(): called when not inited\n");
		return 0;
	}
	
	// load dynamic lib
	sprintf (modpath, "%s/%s%s", CDPDIR, modname, CDPEXT);
	mod = dlopen (modpath, RTLD_NOW);
	if (!mod)
	{
		cdp_last_error = CDPERR_NOT_FOUND;
		return NULL;
	}

	// look it up in already loaded
	for (i = 0, cdp = cdp_modules; cdp->used && cdp->hmodule != mod;
			++cdp, ++i)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !cdp->hmodule)
			newslot = cdp;
	}
	if (i >= MAX_CDPS)
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDPs limit reached while loading %s\n",
				modname);
		dlclose (mod);
		cdp_last_error = CDPERR_TOO_MANY;
		return NULL;
	}

	if (cdp->hmodule)
	{	// module has already been loaded
		cdp->refcount++;
		return cdp;
	}

	dlerror ();	// clear any error
	info = dlsym (mod, CDP_INFO_SYM_NAME);
	if (!info && (errstr = dlerror ()))
	{
		dlclose (mod);
		cdp_last_error = CDPERR_BAD_MODULE;
		return NULL;
	}
	
	if (info->size < CDP_MODINFO_MIN_SIZE || info->api_ver > CDPAPI_VERSION)
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDP %s is invalid or newer API version\n",
				modname);
		dlclose (mod);
		cdp_last_error = CDPERR_UNKNOWN_VER;
		return NULL;
	}

	ihost = cdp_GetInterface (CDPITF_KIND_HOST, info->api_ver);
	if (!ihost)
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDP %s requested unsupported API version 0x%08x\n",
				modname, info->api_ver);
		dlclose (mod);
		cdp_last_error = CDPERR_UNKNOWN_VER;
		return NULL;
	}

	if (!info->module_init ((cdp_Itf_Host*)ihost))
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDP %s failed to init\n",
				modname);
		dlclose (mod);
		cdp_last_error = CDPERR_INIT_FAILED;
		return NULL;
	}

	if (!newslot)
	{
		newslot = cdp;
		newslot->used = true;
	}
	newslot->hmodule = mod;
	newslot->refcount = 1;
	newslot->info = info;
	
	return newslot;
}

cdp_Module*
cdp_CheckModule (cdp_Module* module)
{
	if (module < cdp_modules || module >= cdp_modules + MAX_CDPS ||
			!module->hmodule || !module->info)
		return NULL;
	else
		return module;
}

void
cdp_FreeModule (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);

	if (!modslot)
		return;

	modslot->refcount--;
	if (modslot->refcount == 0)
		modslot->info->module_term ();
	
	dlclose (modslot->hmodule);

	if (modslot->refcount == 0)
	{
		modslot->hmodule = NULL;
		modslot->info = NULL;
	}
}

const char*
cdp_GetModuleName (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (!modslot)
		return "(Error)";
	if (!modslot->info->name)
		return "(Null)";
	return modslot->info->name;
}

uint32
cdp_GetModuleVersion (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (!modslot)
		return 0;
	return (modslot->info->ver_major << 16) | modslot->info->ver_minor;
}

const char*
cdp_GetModuleVersionString (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (!modslot)
		return "(Error)";
	if (!modslot->info->ver_string)
		return "(Null)";
	return modslot->info->ver_string;
}

const char*
cdp_GetModuleComment (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (!modslot)
		return "(Error)";
	if (!modslot->info->comments)
		return "(Null)";
	return modslot->info->comments;
}

// load-all and free-all are here temporarily until
// configs are in place
int
cdp_LoadAllModules (void)
{
	uio_DirList *dirList;
	int nummods = 0;
	int i;

	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_LoadAllModules(): called when not inited\n");
		return 0;
	}

	if (!cdpDir)
		return 0;

	fprintf (stderr, "Loading all CDPs...\n");

	dirList = uio_getDirList (cdpDir, "", CDPEXT, match_MATCH_SUFFIX);
	if (!dirList)
		return 0;

	for (i = 0; i < dirList->numNames; i++)
	{
		char modname[PATH_MAX];
		char* pext;
		cdp_Module* mod;

		fprintf (stderr, "Loading CDP %s...\n", dirList->names[i]);
		strcpy (modname, dirList->names[i]);
		pext = strrchr (modname, '.');
		if (pext) // strip extension
			*pext = 0;

		mod = cdp_LoadModule (modname);
		if (mod)
		{
			nummods++;
			fprintf (stderr, "\tloaded CDP: %s v%s (%s)\n",
					cdp_GetModuleName (mod),
					cdp_GetModuleVersionString (mod),
					cdp_GetModuleComment (mod));
		}
		else
		{
			fprintf (stderr, "\tload failed, error %u\n",
					cdp_GetError ());
		}
	}
	uio_freeDirList (dirList);

	return nummods;
}

void
cdp_FreeAllModules (void)
{
	cdp_Module* cdp;

	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_FreeAllModules(): called when not inited\n");
		return;
	}
	
	for (cdp = cdp_modules; cdp->used; ++cdp)
	{
		if (cdp->hmodule)
			cdp_FreeModule (cdp);
	}
}
