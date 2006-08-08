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
	MEM_HANDLE handle _ALIGNED_ON(sizeof (MEM_HANDLE));
	RACE_DESC data _ALIGNED_ANY;
} CODERES_STRUCT;

MEM_HANDLE
LoadCodeResFile (PSTR pStr)
{
	(void) pStr;  /* Satisfying compiler (unused parameter) */
	return (0);
}

static MEM_HANDLE
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
	MEM_HANDLE hData;

	which_res = GetResFileChar (fp);
	hData = mem_request (sizeof (CODERES_STRUCT));
	if (hData)
	{
		RACE_DESCPTR RDPtr;

		RDPtr = 0;
		switch (which_res)
		{
			case ANDROSYN_CODE_RES:
			{
				extern RACE_DESCPTR init_androsynth (void);

				RDPtr = init_androsynth ();
				break;
			}
			case ARILOU_CODE_RES:
			{
				extern RACE_DESCPTR init_arilou (void);

				RDPtr = init_arilou ();
				break;
			}
			case BLACKURQ_CODE_RES:
			{
				extern RACE_DESCPTR init_black_urquan (void);

				RDPtr = init_black_urquan ();
				break;
			}
			case CHENJESU_CODE_RES:
			{
				extern RACE_DESCPTR init_chenjesu (void);

				RDPtr = init_chenjesu ();
				break;
			}
			case CHMMR_CODE_RES:
			{
				extern RACE_DESCPTR init_chmmr (void);

				RDPtr = init_chmmr ();
				break;
			}
			case DRUUGE_CODE_RES:
			{
				extern RACE_DESCPTR init_druuge (void);

				RDPtr = init_druuge ();
				break;
			}
			case HUMAN_CODE_RES:
			{
				extern RACE_DESCPTR init_human (void);

				RDPtr = init_human ();
				break;
			}
			case ILWRATH_CODE_RES:
			{
				
				extern RACE_DESCPTR init_ilwrath (void);

				RDPtr = init_ilwrath ();
				break;
			}
			case MELNORME_CODE_RES:
			{
				extern RACE_DESCPTR init_melnorme (void);

				RDPtr = init_melnorme ();
				break;
			}
			case MMRNMHRM_CODE_RES:
			{
				extern RACE_DESCPTR init_mmrnmhrm (void);

				RDPtr = init_mmrnmhrm ();
				break;
			}
			case MYCON_CODE_RES:
			{
				extern RACE_DESCPTR init_mycon (void);

				RDPtr = init_mycon ();
				break;
			}
			case ORZ_CODE_RES:
			{
				extern RACE_DESCPTR init_orz (void);

				RDPtr = init_orz ();
				break;
			}
			case PKUNK_CODE_RES:
			{
				extern RACE_DESCPTR init_pkunk (void);

				RDPtr = init_pkunk ();
				break;
			}
			case SHOFIXTI_CODE_RES:
			{
				extern RACE_DESCPTR init_shofixti (void);

				RDPtr = init_shofixti ();
				break;
			}
			case SLYLANDR_CODE_RES:
			{
				
				extern RACE_DESCPTR init_slylandro (void);

				RDPtr = init_slylandro ();
				break;
			}
			case SPATHI_CODE_RES:
			{
				extern RACE_DESCPTR init_spathi (void);

				RDPtr = init_spathi ();
				break;
			}
			case SUPOX_CODE_RES:
			{
				extern RACE_DESCPTR init_supox (void);

				RDPtr = init_supox ();
				break;
			}
			case SYREEN_CODE_RES:
			{
				extern RACE_DESCPTR init_syreen (void);

				RDPtr = init_syreen ();
				break;
			}
			case THRADD_CODE_RES:
			{
				extern RACE_DESCPTR init_thraddash (void);

				RDPtr = init_thraddash ();
				break;
			}
			case UMGAH_CODE_RES:
			{
				extern RACE_DESCPTR init_umgah (void);

				RDPtr = init_umgah ();
				break;
			}
			case URQUAN_CODE_RES:
			{
				extern RACE_DESCPTR init_urquan (void);

				RDPtr = init_urquan ();
				break;
			}
			case UTWIG_CODE_RES:
			{
				extern RACE_DESCPTR init_utwig (void);

				RDPtr = init_utwig ();
				break;
			}
			case VUX_CODE_RES:
			{
				
				extern RACE_DESCPTR init_vux (void);

				RDPtr = init_vux ();
				break;
			}
			case YEHAT_CODE_RES:
			{
				extern RACE_DESCPTR init_yehat (void);

				RDPtr = init_yehat ();
				break;
			}
			case ZOQFOT_CODE_RES:
			{
				extern RACE_DESCPTR init_zoqfotpik (void);

				RDPtr = init_zoqfotpik ();
				break;
			}
			case SAMATRA_CODE_RES:
			{
				extern RACE_DESCPTR init_samatra (void);

				RDPtr = init_samatra ();
				break;
			}
			case SIS_CODE_RES:
			{
				extern RACE_DESCPTR init_sis (void);

				RDPtr = init_sis ();
				break;
			}
			case PROBE_CODE_RES:
			{
				extern RACE_DESCPTR init_probe (void);

				RDPtr = init_probe ();
				break;
			}
		}

		if (RDPtr == 0)
		{
			mem_release (hData);
			hData = 0;
		}
		else
		{
			CODERES_STRUCT *cs;

			cs = (CODERES_STRUCT *) mem_lock (hData);
			cs->data = *RDPtr;
					// Structure assignment.
			mem_unlock (hData);
		}
	}
	(void) length;  /* Satisfying compiler (unused parameter) */
	return (hData);
}


BOOLEAN
InstallCodeResType (COUNT code_type)
{
	return (InstallResTypeVectors (code_type,
			GetCodeResData, mem_release));
}


MEM_HANDLE
LoadCodeResInstance (DWORD res)
{
	MEM_HANDLE hData;

	hData = res_GetResource (res);
	if (hData)
		res_DetachResource (res);

	return (hData);
}


BOOLEAN
DestroyCodeRes (MEM_HANDLE hCode)
{
	return (mem_release (hCode));
}


PVOID
CaptureCodeRes (MEM_HANDLE hCode, PVOID pData, PVOID *ppLocData)
{
	CODERES_STRUCT *cs;

	if (hCode == 0)
	{
		log_add (log_Fatal, "dummy.c::CaptureCodeRes() hCode==0! FATAL!");
		return(0);
	}

	cs = (CODERES_STRUCT *) mem_lock (hCode);
	cs->handle = hCode;
	*ppLocData = (void *) &cs->data;

	(void) pData;  /* Satisfying compiler (unused parameter) */
	return (void *) cs;
}


MEM_HANDLE
ReleaseCodeRes (PVOID CodeRef)
{
	if (CodeRef)
	{
		MEM_HANDLE hCode;

		mem_unlock (hCode = *(MEM_HANDLE *)CodeRef);
		return (hCode);
	}

	return (0);
}

DRAWABLE
CreatePixmapRegion (FRAME Frame, PPOINT lpOrg, SIZE width, SIZE height)
{
	(void) lpOrg;  /* Satisfying compiler (unused parameter) */
	(void) width;  /* Satisfying compiler (unused parameter) */
	(void) height;  /* Satisfying compiler (unused parameter) */
	return (GetFrameHandle (Frame));
}


