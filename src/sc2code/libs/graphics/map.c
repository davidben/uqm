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

#include "gfxintrn.h"

MAP_TYPE
SetContextMapType (MAP_TYPE MapType)
{
#ifndef MAPPING
	return (MAP_NOXFORM);
#else /* MAPPING */
	MAP_TYPE oldMapType;

	if (!ContextActive ())
		oldMapType = MAP_NOXFORM;
	else if ((oldMapType = _get_context_map_type ()) != MapType)
	{
		SwitchContextMapType (MapType);
	}

	return (oldMapType);
#endif /* MAPPING */
}

BOOLEAN
SetContextWinOrigin (PPOINT lpOrg)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive ())
		return (FALSE);

	SwitchContextWinOrigin (lpOrg);

	return (TRUE);
#endif /* MAPPING */
}

BOOLEAN
SetContextViewExtents (PEXTENT lpExtent)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive ())
		return (FALSE);

	SwitchContextViewExtents (lpExtent);

	return (TRUE);
#endif /* MAPPING */
}

BOOLEAN
SetContextWinExtents (PEXTENT lpExtent)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive ())
		return (FALSE);

	SwitchContextWinExtents (lpExtent);

	return (TRUE);
#endif /* MAPPING */
}

BOOLEAN
GetContextWinOrigin (PPOINT lpOrg)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive ())
		return (FALSE);

	*lpOrg = _pCurContext->Map.WinOrg;

	return (TRUE);
#endif /* MAPPING */
}

BOOLEAN
GetContextViewExtents (PEXTENT lpExtent)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive ())
		return (FALSE);

	*lpExtent = _pCurContext->Map.ViewExt;

	return (TRUE);
#endif /* MAPPING */
}

BOOLEAN
GetContextWinExtents (PEXTENT lpExtent)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive ())
		return (FALSE);

	*lpExtent = _pCurContext->Map.WinExt;

	return (TRUE);
#endif /* MAPPING */
}

BOOLEAN
LOGtoDEV (PPOINT lpSrcPt, PPOINT lpDstPt, COUNT NumPoints)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive () || NumPoints == 0)
		return (FALSE);

	{
		MAP_TYPE MapType;

		_init_context_map (&MapType);
		if (MapType == MAP_NOXFORM)
		{
			do
				*lpDstPt++ = *lpSrcPt++;
			while (--NumPoints);
		}
		else
		{
			do
			{
				lpDstPt->x = LXtoDX (lpSrcPt->x);
				lpDstPt->y = LYtoDY (lpSrcPt->y);
				++lpSrcPt;
				++lpDstPt;
			} while (--NumPoints);
		}
		_uninit_context_map ();

		return (TRUE);
	}
#endif /* MAPPING */
}

BOOLEAN
DEVtoLOG (PPOINT lpSrcPt, PPOINT lpDstPt, COUNT NumPoints)
{
#ifndef MAPPING
	return (FALSE);
#else /* MAPPING */
	if (!ContextActive () || NumPoints == 0)
		return (FALSE);

	{
		MAP_TYPE MapType;

		_init_context_map (&MapType);
		if (MapType == MAP_NOXFORM)
		{
			do
				*lpDstPt++ = *lpSrcPt++;
			while (--NumPoints);
		}
		else
		{
			do
			{
				lpDstPt->x = DXtoLX (lpSrcPt->x);
				lpDstPt->y = DYtoLY (lpSrcPt->y);
				++lpSrcPt;
				++lpDstPt;
			} while (--NumPoints);
		}
		_uninit_context_map ();

		return (TRUE);
	}
#endif /* MAPPING */
}

