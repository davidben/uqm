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

#include "starcon.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void
InitPlanetInfo (void)
{
	PVOID fp;

	fp = OpenResFile (STARINFO_FILE, "wb");
	if (fp)
	{
		DWORD offset;
		STAR_DESCPTR pSD;

		offset = 0;
		pSD = &star_array[0];
		do
		{
			WriteResFile (&offset, sizeof (offset), 1, fp);
			++pSD;
		} while (pSD->star_pt.x <= MAX_X_UNIVERSE
				&& pSD->star_pt.y <= MAX_Y_UNIVERSE);

		CloseResFile (fp);
	}
}

void
UninitPlanetInfo (void)
{
	unlink (STARINFO_FILE);
}

void
GetPlanetInfo (void)
{
	PVOID fp;

	pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[BIOLOGICAL_SCAN] =
			pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[MINERAL_SCAN] =
			pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN] = 0;
	fp = OpenResFile (STARINFO_FILE, "rb");
	if (fp)
	{
		COUNT star_index, planet_index, moon_index;
		DWORD offset;

		star_index = (COUNT)(CurStarDescPtr - star_array);
		planet_index = (COUNT)(pSolarSysState->pBaseDesc->pPrevDesc
				- pSolarSysState->PlanetDesc);
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == pSolarSysState->SunDesc)
			moon_index = 0;
		else
			moon_index = (COUNT)(pSolarSysState->pOrbitalDesc
					- pSolarSysState->MoonDesc + 1);

		SeekResFile (fp, star_index * sizeof (offset), SEEK_SET);
		ReadResFile (&offset, sizeof (offset), 1, fp);

		if (offset)
		{
			 COUNT i;

			 for (i = 0; i < planet_index; ++i)
				offset += sizeof (
						pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask
						) * (pSolarSysState->PlanetDesc[i].NumPlanets + 1);
				
			 offset += sizeof (
					pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask
					) * moon_index;

			 SeekResFile (fp, offset, SEEK_SET);
			 ReadResFile (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask,
						sizeof (
						pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask
						), 1, fp);
		}

		CloseResFile (fp);
	}
}

void
PutPlanetInfo (void)
{
	PVOID fp;

	fp = OpenResFile (STARINFO_FILE, "r+b");
	if (fp)
	{
		COUNT i;
		COUNT star_index, planet_index, moon_index;
		DWORD offset;

		star_index = (COUNT)(CurStarDescPtr - star_array);
		planet_index = (COUNT)(pSolarSysState->pBaseDesc->pPrevDesc
				- pSolarSysState->PlanetDesc);
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == pSolarSysState->SunDesc)
			moon_index = 0;
		else
			moon_index = (COUNT)(pSolarSysState->pOrbitalDesc
					- pSolarSysState->MoonDesc + 1);

		SeekResFile (fp, star_index * sizeof (offset), SEEK_SET);
		ReadResFile (&offset, sizeof (offset), 1, fp);

		if (offset == 0)
		{
			DWORD ScanRetrieveMask[NUM_SCAN_TYPES] =
			{
				0, 0, 0,
			};

			offset = LengthResFile (fp);

			SeekResFile (fp, star_index * sizeof (offset), SEEK_SET);
			WriteResFile (&offset, sizeof (offset), 1, fp);

			SeekResFile (fp, offset, SEEK_SET);
			for (i = 0; i < pSolarSysState->SunDesc[0].NumPlanets; ++i)
			{
				COUNT j;

				WriteResFile (ScanRetrieveMask, sizeof (ScanRetrieveMask), 1, fp);
				for (j = 0; j < pSolarSysState->PlanetDesc[i].NumPlanets; ++j)
					WriteResFile (ScanRetrieveMask, sizeof (ScanRetrieveMask), 1, fp);
			}
		}

		for (i = 0; i < planet_index; ++i)
			offset += sizeof (
					pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask
					) * (pSolarSysState->PlanetDesc[i].NumPlanets + 1);
				
		offset += sizeof (
				pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask
				) * moon_index;

		SeekResFile (fp, offset, SEEK_SET);
		WriteResFile (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask,
				sizeof (
				pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask
				), 1, fp);

		CloseResFile (fp);
	}
}

