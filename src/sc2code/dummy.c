//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This file seems to be a collection of functions that don't do
 * much.
 */

#include "coderes.h"
#include "globdata.h"
#include "races.h"

#include "libs/compiler.h"
#include "libs/log.h"

#include <ctype.h>


typedef struct
{
	RACE_DESC data _ALIGNED_ANY;
} CODERES_STRUCT;

void *
LoadCodeResFile (const char *pStr)
{
	(void) pStr;  /* Satisfying compiler (unused parameter) */
	return NULL;
}

static void *
GetCodeResData (uio_Stream *fp, DWORD length)
{
enum
{
	ANDROSYN_CODE_RES,
	ARILOU_CODE_RES,
	BLACKURQ_CODE_RES,
	CHENJESU_CODE_RES,
	CHMMR_CODE_RES,
	DRUUGE_CODE_RES,
	HUMAN_CODE_RES,
	ILWRATH_CODE_RES,
	MELNORME_CODE_RES,
	MMRNMHRM_CODE_RES,
	MYCON_CODE_RES,
	ORZ_CODE_RES,
	PKUNK_CODE_RES,
	SHOFIXTI_CODE_RES,
	SLYLANDR_CODE_RES,
	SPATHI_CODE_RES,
	SUPOX_CODE_RES,
	SYREEN_CODE_RES,
	THRADD_CODE_RES,
	UMGAH_CODE_RES,
	URQUAN_CODE_RES,
	UTWIG_CODE_RES,
	VUX_CODE_RES,
	YEHAT_CODE_RES,
	ZOQFOT_CODE_RES,

	SAMATRA_CODE_RES,
	SIS_CODE_RES,
	PROBE_CODE_RES
};

	BYTE which_res;
	void *hData;

	which_res = GetResFileChar (fp);
	hData = HMalloc (sizeof (CODERES_STRUCT));
	if (hData)
	{
		RACE_DESC *RDPtr;

		RDPtr = NULL;
		switch (which_res)
		{
			case ANDROSYN_CODE_RES:
			{
				extern RACE_DESC* init_androsynth (void);

				RDPtr = init_androsynth ();
				break;
			}
			case ARILOU_CODE_RES:
			{
				extern RACE_DESC* init_arilou (void);

				RDPtr = init_arilou ();
				break;
			}
			case BLACKURQ_CODE_RES:
			{
				extern RACE_DESC* init_black_urquan (void);

				RDPtr = init_black_urquan ();
				break;
			}
			case CHENJESU_CODE_RES:
			{
				extern RACE_DESC* init_chenjesu (void);

				RDPtr = init_chenjesu ();
				break;
			}
			case CHMMR_CODE_RES:
			{
				extern RACE_DESC* init_chmmr (void);

				RDPtr = init_chmmr ();
				break;
			}
			case DRUUGE_CODE_RES:
			{
				extern RACE_DESC* init_druuge (void);

				RDPtr = init_druuge ();
				break;
			}
			case HUMAN_CODE_RES:
			{
				extern RACE_DESC* init_human (void);

				RDPtr = init_human ();
				break;
			}
			case ILWRATH_CODE_RES:
			{
				
				extern RACE_DESC* init_ilwrath (void);

				RDPtr = init_ilwrath ();
				break;
			}
			case MELNORME_CODE_RES:
			{
				extern RACE_DESC* init_melnorme (void);

				RDPtr = init_melnorme ();
				break;
			}
			case MMRNMHRM_CODE_RES:
			{
				extern RACE_DESC* init_mmrnmhrm (void);

				RDPtr = init_mmrnmhrm ();
				break;
			}
			case MYCON_CODE_RES:
			{
				extern RACE_DESC* init_mycon (void);

				RDPtr = init_mycon ();
				break;
			}
			case ORZ_CODE_RES:
			{
				extern RACE_DESC* init_orz (void);

				RDPtr = init_orz ();
				break;
			}
			case PKUNK_CODE_RES:
			{
				extern RACE_DESC* init_pkunk (void);

				RDPtr = init_pkunk ();
				break;
			}
			case SHOFIXTI_CODE_RES:
			{
				extern RACE_DESC* init_shofixti (void);

				RDPtr = init_shofixti ();
				break;
			}
			case SLYLANDR_CODE_RES:
			{
				
				extern RACE_DESC* init_slylandro (void);

				RDPtr = init_slylandro ();
				break;
			}
			case SPATHI_CODE_RES:
			{
				extern RACE_DESC* init_spathi (void);

				RDPtr = init_spathi ();
				break;
			}
			case SUPOX_CODE_RES:
			{
				extern RACE_DESC* init_supox (void);

				RDPtr = init_supox ();
				break;
			}
			case SYREEN_CODE_RES:
			{
				extern RACE_DESC* init_syreen (void);

				RDPtr = init_syreen ();
				break;
			}
			case THRADD_CODE_RES:
			{
				extern RACE_DESC* init_thraddash (void);

				RDPtr = init_thraddash ();
				break;
			}
			case UMGAH_CODE_RES:
			{
				extern RACE_DESC* init_umgah (void);

				RDPtr = init_umgah ();
				break;
			}
			case URQUAN_CODE_RES:
			{
				extern RACE_DESC* init_urquan (void);

				RDPtr = init_urquan ();
				break;
			}
			case UTWIG_CODE_RES:
			{
				extern RACE_DESC* init_utwig (void);

				RDPtr = init_utwig ();
				break;
			}
			case VUX_CODE_RES:
			{
				
				extern RACE_DESC* init_vux (void);

				RDPtr = init_vux ();
				break;
			}
			case YEHAT_CODE_RES:
			{
				extern RACE_DESC* init_yehat (void);

				RDPtr = init_yehat ();
				break;
			}
			case ZOQFOT_CODE_RES:
			{
				extern RACE_DESC* init_zoqfotpik (void);

				RDPtr = init_zoqfotpik ();
				break;
			}
			case SAMATRA_CODE_RES:
			{
				extern RACE_DESC* init_samatra (void);

				RDPtr = init_samatra ();
				break;
			}
			case SIS_CODE_RES:
			{
				extern RACE_DESC* init_sis (void);

				RDPtr = init_sis ();
				break;
			}
			case PROBE_CODE_RES:
			{
				extern RACE_DESC* init_probe (void);

				RDPtr = init_probe ();
				break;
			}
		}

		if (RDPtr == 0)
		{
			HFree (hData);
			hData = 0;
		}
		else
		{
			CODERES_STRUCT *cs;

			cs = (CODERES_STRUCT *) hData;
			cs->data = *RDPtr;  // Structure assignment.
		}
	}
	(void) length;  /* Satisfying compiler (unused parameter) */
	return (hData);
}

BOOLEAN
_ReleaseCodeResData (void *data)
{
	HFree (data);
	return TRUE;
}

static void *
GetCodeFileData (const char *pathname)
{
	return LoadResourceFromPath (pathname, GetCodeResData);
}

BOOLEAN
InstallCodeResType ()
{
	return (InstallResTypeVectors ("CODE",
			GetCodeFileData, _ReleaseCodeResData));
}


void *
LoadCodeResInstance (RESOURCE res)
{
	void *hData;

	hData = res_GetResource (res);
	if (hData)
		res_DetachResource (res);

	return (hData);
}


BOOLEAN
DestroyCodeRes (void *hCode)
{
	HFree (hCode);
	return (TRUE);
}


void*
CaptureCodeRes (void *hCode, void *pData, void **ppLocData)
{
	CODERES_STRUCT *cs;

	if (hCode == NULL)
	{
		log_add (log_Fatal, "dummy.c::CaptureCodeRes() hCode==NULL! FATAL!");
		return(NULL);
	}

	cs = (CODERES_STRUCT *) hCode;
	*ppLocData = &cs->data;

	(void) pData;  /* Satisfying compiler (unused parameter) */
	return cs;
}


void *
ReleaseCodeRes (void *CodeRef)
{
	return CodeRef;
}

