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

#include "encount.h"
#include "globdata.h"
#include "lander.h"
#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "planets/genall.h"
#include "libs/mathlib.h"


void
GenerateShipVault (BYTE control)
{
	switch (control)
	{
		case GENERATE_ENERGY:
			if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[0]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[0])
			{
				DWORD rand_val, old_rand;

				old_rand = TFB_SeedRandom (
						pSolarSysState->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN]
						);

				rand_val = TFB_Random ();
				pSolarSysState->SysInfo.PlanetInfo.CurPt.x =
						(LOBYTE (LOWORD (rand_val)) % (MAP_WIDTH - (8 << 1))) + 8;
				pSolarSysState->SysInfo.PlanetInfo.CurPt.y =
						(HIBYTE (LOWORD (rand_val)) % (MAP_HEIGHT - (8 << 1))) + 8;
				pSolarSysState->SysInfo.PlanetInfo.CurDensity = 0;
				if (!GET_GAME_STATE (SHIP_VAULT_UNLOCKED))
					pSolarSysState->SysInfo.PlanetInfo.CurType = 0;
				else
					pSolarSysState->SysInfo.PlanetInfo.CurType = 1;
				pSolarSysState->CurNode = 1;
				if (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
						& (1L << 0))
				{
					pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
							&= ~(1L << 0);
					if (GET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP))
					{
						((PPLANETSIDE_DESC)pMenuState->ModuleFrame)->InTransit = TRUE;

						SET_GAME_STATE (SHIP_VAULT_UNLOCKED, 1);
						SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 0);
						SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
					}
					else if (!GET_GAME_STATE (KNOW_SYREEN_VAULT))
					{
						SET_GAME_STATE (KNOW_SYREEN_VAULT, 1);
						SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
					}
				}

				TFB_SeedRandom (old_rand);
				break;
			}
			pSolarSysState->CurNode = 0;
			break;
		case GENERATE_ORBITAL:
			if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[0]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[0])
			{
				LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
				pSolarSysState->PlanetSideFrame[1] =
						CaptureDrawable (
								LoadGraphic (VAULT_MASK_PMAP_ANIM)
								);
				pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
						CaptureStringTable (
								LoadStringTable (VAULT_STRTAB)
								);
				if (GET_GAME_STATE (SHIP_VAULT_UNLOCKED))
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							SetAbsStringTableIndex (
							pSolarSysState->SysInfo.PlanetInfo.DiscoveryString,
							2
							);
				else if (GET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP))
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							SetAbsStringTableIndex (
							pSolarSysState->SysInfo.PlanetInfo.DiscoveryString,
							1
							);
			}
		default:
			GenerateRandomIP (control);
			break;
	}
}


