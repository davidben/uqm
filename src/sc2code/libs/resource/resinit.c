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

#include "resintrn.h"

static MEM_HANDLE hIndexList;

static void
_add_index_list (MEM_HANDLE hRH)
{
#ifndef PACKAGING
	INDEX_HEADERPTR ResHeaderPtr;

	ResHeaderPtr = LockResourceHeader (hRH);
	strncpy (ResHeaderPtr->index_file_name, _cur_resfile_name,
			sizeof (ResHeaderPtr->index_file_name) - 1);
	ResHeaderPtr->hPredHeader = 0;
	ResHeaderPtr->hSuccHeader = hIndexList;
	if (ResHeaderPtr->hSuccHeader)
	{
		INDEX_HEADERPTR SuccResHeaderPtr;

		SuccResHeaderPtr = LockResourceHeader (hIndexList);
		SuccResHeaderPtr->hPredHeader = hRH;
		UnlockResourceHeader (hIndexList);
	}
	UnlockResourceHeader (hRH);
#endif /* PACKAGING */

	hIndexList = hRH;
}

static void
_sub_index_list (MEM_HANDLE hRH)
{
#ifdef PACKAGING
	hIndexList = 0;
#else /* !PACKAGING */
	INDEX_HEADERPTR ResHeaderPtr;
	MEM_HANDLE hPred, hSucc;

	ResHeaderPtr = LockResourceHeader (hRH);

	hPred = ResHeaderPtr->hPredHeader;
	hSucc = ResHeaderPtr->hSuccHeader;
	if (hRH == hIndexList)
		hIndexList = hSucc;

	if (hPred)
	{
		INDEX_HEADERPTR PredResHeaderPtr;

		PredResHeaderPtr = LockResourceHeader (hPred);
		PredResHeaderPtr->hSuccHeader = hSucc;
		UnlockResourceHeader (hPred);
	}
	if (hSucc)
	{
		INDEX_HEADERPTR SuccResHeaderPtr;

		SuccResHeaderPtr = LockResourceHeader (hSucc);
		SuccResHeaderPtr->hPredHeader = hPred;
		UnlockResourceHeader (hSucc);
	}

	UnlockResourceHeader (hRH);
#endif /* PACKAGING */
}

MEM_HANDLE
_GetResFileData (FILE *res_fp, DWORD flen)
{
	UWORD lo_word, hi_word;
	DWORD res_offs, remainder;
	MEM_SIZE HeaderSize;
	INDEX_HEADER h;
	INDEX_HEADERPTR ResHeaderPtr;
	MEM_HANDLE hRH;
	char buf[32];

	res_offs = TellResFile (res_fp);

	ReadResFile (buf, 1, 22, res_fp);

	h.res_fp = res_fp;
	h.res_flags = MAKE_WORD (buf[0], buf[1]) ? IS_PACKAGED : 0;

	lo_word = MAKE_WORD (buf[2], buf[3]);
	hi_word = MAKE_WORD (buf[4], buf[5]);
	h.packmem_list_offs = MAKE_DWORD (lo_word, hi_word);

	lo_word = MAKE_WORD (buf[6], buf[7]);
	hi_word = MAKE_WORD (buf[8], buf[9]);
	h.path_list_offs = MAKE_DWORD (lo_word, hi_word);

	lo_word = MAKE_WORD (buf[10], buf[11]);
	hi_word = MAKE_WORD (buf[12], buf[13]);
	h.file_list_offs = MAKE_DWORD (lo_word, hi_word);

	h.num_packages = (RES_PACKAGE)MAKE_WORD (buf[14], buf[15]);
	h.num_types = (RES_TYPE)MAKE_WORD (buf[16], buf[17]);

	lo_word = MAKE_WORD (buf[18], buf[19]);
	hi_word = MAKE_WORD (buf[20], buf[21]);
	h.index_info.header_len = MAKE_DWORD (lo_word, hi_word);

	HeaderSize = (MEM_SIZE)(sizeof (INDEX_HEADER)
			+ (sizeof (PACKAGE_DESC) * h.num_packages)
			+ (sizeof (TYPE_DESC) * h.num_types));
	if (h.res_flags & IS_PACKAGED)
	{
		DWORD offs;

		remainder = h.index_info.header_len - h.packmem_list_offs;
		offs = HeaderSize - h.packmem_list_offs;
		HeaderSize += remainder;
		h.packmem_list_offs += offs;
		h.file_list_offs += offs;
		h.path_list_offs += offs;
	}

#ifndef PACKAGING
	{
		MEM_HANDLE hNextRH;

		h.data_offs = res_offs;
		for (hRH = hIndexList; hRH != 0; hRH = hNextRH)
		{
			ResHeaderPtr = LockResourceHeader (hRH);
			if (h.data_offs == ResHeaderPtr->data_offs
					&& strncmp (ResHeaderPtr->index_file_name,
					_cur_resfile_name,
					sizeof (ResHeaderPtr->index_file_name) - 1) == 0)
				return (hRH); /* DON'T UNLOCK IT */

			hNextRH = ResHeaderPtr->hSuccHeader;
			UnlockResourceHeader (hRH);
		}
	}
#endif /* PACKAGING */

	if ((hRH = AllocResourceHeader (HeaderSize))
			&& (ResHeaderPtr = LockResourceHeader (hRH)))
	{
		*ResHeaderPtr = h;

		{
			RES_PACKAGE p;

			ResHeaderPtr->PackageList = (PPACKAGE_DESC)&ResHeaderPtr[1];
			for (p = 0; p < ResHeaderPtr->num_packages; ++p)
			{
				ReadResFile (buf, 1, 8, res_fp);

				lo_word = MAKE_WORD (buf[0], buf[1]);
				hi_word = MAKE_WORD (buf[2], buf[3]);
				ResHeaderPtr->PackageList[p].packmem_info =
						(RESOURCE)MAKE_DWORD (lo_word, hi_word);

				lo_word = MAKE_WORD (buf[4], buf[5]);
				hi_word = MAKE_WORD (buf[6], buf[7]);
				ResHeaderPtr->PackageList[p].flags_and_data_loc =
						MAKE_DWORD (lo_word, hi_word) + res_offs;
			}
		}

		{
			RES_TYPE t;
			INDEX_HEADERPTR CurResHeaderPtr;

			CurResHeaderPtr = _get_current_index_header ();

			ResHeaderPtr->TypeList =
					(PTYPE_DESC)&ResHeaderPtr->PackageList[ResHeaderPtr->num_packages];
			for (t = 0; t < ResHeaderPtr->num_types; ++t)
			{
				ReadResFile (buf, 1, 2, res_fp);

				ResHeaderPtr->TypeList[t].instance_count =
						MAKE_WORD (buf[0], buf[1]);
				if (CurResHeaderPtr)
					ResHeaderPtr->TypeList[t].func_vectors =
							CurResHeaderPtr->TypeList[t].func_vectors;
			}
		}

		if (h.res_flags & IS_PACKAGED)
			ReadResFile (&ResHeaderPtr->TypeList[ResHeaderPtr->num_types],
					1, (COUNT)remainder, res_fp);

		_add_index_list (hRH);

		return (hRH); /* DON'T UNLOCK IT */
	}

	FreeResourceHeader (hRH);
	(void) flen;  /* Satisfy compiler (unused parameter) */
	return (NULL_HANDLE);
}

MEM_HANDLE
InitResourceSystem (PVOID resfile, COUNT resindex_type, BOOLEAN
		(*FileErrorFunc) (PVOID filename))
{
	MEM_HANDLE h;

	h = OpenResourceIndexFile (resfile);
	if (h)
	{
		SetResourceIndex (h);

		InstallResTypeVectors (resindex_type,
				_GetResFileData, FreeResourceHeader);
	}

		(void) FileErrorFunc;  /* Satisfy compiler (unused parameter) */
	return (h);
}

BOOLEAN
UninitResourceSystem (void)
{
	if (hIndexList)
	{
		do
			CloseResourceIndex (hIndexList);
		while (hIndexList);

		return (TRUE);
	}

	return (FALSE);
}

MEM_HANDLE
OpenResourceIndexFile (PVOID resfile)
{
	FILE *res_fp;
	char fullname[256];

	strcpy (fullname, resfile);
	if ((res_fp = OpenResFile (fullname, "rb")) == 0)
	{
		sprintf (fullname, "%s.pkg", (char *) resfile);
		if ((res_fp = OpenResFile (fullname, "rb")) == 0)
		{
			sprintf (fullname, "%s.ndx", (char *) resfile);
			res_fp = OpenResFile (fullname, "rb");
		}
	}

	if (res_fp)
	{
		MEM_HANDLE hRH;

		_cur_resfile_name = fullname;

		hRH = _GetResFileData (res_fp, LengthResFile (res_fp));
		/* DO NOT CloseResFile!!! */

		return (hRH);
	}

	return (0);
}

MEM_HANDLE
OpenResourceIndexInstance (DWORD res)
{
	MEM_HANDLE hRH;

	hRH = GetResource (res);
	if (hRH)
		DetachResource (res);

	return (hRH);
}

MEM_HANDLE
SetResourceIndex (MEM_HANDLE hRH)
{
	MEM_HANDLE hOldRH;
	static MEM_HANDLE hCurResHeader;

	if ((hOldRH = hCurResHeader) != hRH)
	{
		INDEX_HEADERPTR ResHeaderPtr;

		ResHeaderPtr = LockResourceHeader (hRH);
		UnlockResourceHeader (hRH);
		_set_current_index_header (ResHeaderPtr);
		hCurResHeader = hRH;
	}

	return (hOldRH);
}

BOOLEAN
CloseResourceIndex (MEM_HANDLE hRH)
{
	if (UnlockResourceHeader (hRH))
	{
		FILE *res_fp;
		INDEX_HEADERPTR ResHeaderPtr;

		ResHeaderPtr = LockResourceHeader (hRH);
#ifdef DEBUG
		{
			COUNT i;

			for (i = 0; i < ResHeaderPtr->num_packages; ++i)
			{
				UWORD hi;

				hi = HIWORD (ResHeaderPtr->PackageList[i].flags_and_data_loc);
				if (HIBYTE (hi) == 0)
				{
					MEM_HANDLE hList;
					RES_HANDLE_LISTPTR ResourceHandleListPtr;
					ENCODEPTR TypeEncodePtr;
					DATAPTR DataPtr;

					hList = (MEM_HANDLE)LOWORD (
							ResHeaderPtr->PackageList[i].flags_and_data_loc);
					LockResourceHandleList (ResHeaderPtr, hList, i + 1,
							&ResourceHandleListPtr, &TypeEncodePtr, &DataPtr);
					fprintf (stderr, "Package %u has %u instances left\n",
							i + 1, ResourceHandleListPtr->num_valid_handles);
					UnlockResourceHandleList (hList);
				}
			}
		}
#endif /* DEBUG */

		_sub_index_list (hRH);
		res_fp = ResHeaderPtr->res_fp;
		if (ResHeaderPtr == _get_current_index_header ())
			SetResourceIndex (hIndexList);
		UnlockResourceHeader (hRH);
		FreeResourceHeader (hRH);

		if (res_fp)
		{
			MEM_HANDLE hNextRH;

			for (hRH = hIndexList; hRH && res_fp; hRH = hNextRH)
			{
				ResHeaderPtr = LockResourceHeader (hRH);
				if (res_fp == ResHeaderPtr->res_fp)
					res_fp = 0;
				hNextRH = ResHeaderPtr->hSuccHeader;
				UnlockResourceHeader (hRH);
			}

			if (res_fp)
				CloseResFile (res_fp);
		}

		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
InstallResTypeVectors (COUNT res_type, 
		MEM_HANDLE (*load_func) (FILE *fp, DWORD len),
		BOOLEAN (*free_func) (MEM_HANDLE handle))
{
	INDEX_HEADERPTR ResHeaderPtr;

	ResHeaderPtr = _get_current_index_header ();
	if (ValidResType (ResHeaderPtr, res_type))
	{
		ResHeaderPtr->TypeList[res_type - 1].func_vectors.load_func = load_func;
		ResHeaderPtr->TypeList[res_type - 1].func_vectors.free_func = free_func;

		return (TRUE);
	}

	return (FALSE);
}

COUNT
CountResourceTypes (void)
{
	INDEX_HEADERPTR ResHeaderPtr;

	ResHeaderPtr = _get_current_index_header ();
	return ((COUNT)ResHeaderPtr->num_types);
}

