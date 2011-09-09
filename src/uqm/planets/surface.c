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

#include "lifeform.h"
#include "planets.h"
#include "libs/mathlib.h"
#include "libs/log.h"


//#define DEBUG_SURFACE

const BYTE *Elements;
const PlanetFrame *PlanData;

static COUNT
CalcMineralDeposits (SYSTEM_INFO *SysInfoPtr, COUNT which_deposit)
{
	BYTE j;
	COUNT num_deposits;
	const ELEMENT_ENTRY *eptr;

	eptr = &SysInfoPtr->PlanetInfo.PlanDataPtr->UsefulElements[0];
	num_deposits = 0;
	j = NUM_USEFUL_ELEMENTS;
	do
	{
		BYTE num_possible;

		num_possible = LOBYTE (RandomContext_Random (SysGenRNG))
				% (DEPOSIT_QUANTITY (eptr->Density) + 1);
		while (num_possible--)
		{
#define MEDIUM_DEPOSIT_THRESHOLD 150
#define LARGE_DEPOSIT_THRESHOLD 225
			COUNT deposit_quality_fine;
			COUNT deposit_quality_gross;

			deposit_quality_fine = (LOWORD (RandomContext_Random (SysGenRNG)) % 100)
					+ (
					DEPOSIT_QUALITY (eptr->Density)
					+ SysInfoPtr->StarSize
					) * 50;
			if (deposit_quality_fine < MEDIUM_DEPOSIT_THRESHOLD)
				deposit_quality_gross = 0;
			else if (deposit_quality_fine < LARGE_DEPOSIT_THRESHOLD)
				deposit_quality_gross = 1;
			else
				deposit_quality_gross = 2;

			GenerateRandomLocation (SysInfoPtr);

			SysInfoPtr->PlanetInfo.CurDensity =
					MAKE_WORD (
					deposit_quality_gross, deposit_quality_fine / 10 + 1
					);
			SysInfoPtr->PlanetInfo.CurType = eptr->ElementType;
#ifdef DEBUG_SURFACE
			log_add (log_Debug, "\t\t%d units of %Fs",
					SysInfoPtr->PlanetInfo.CurDensity,
					Elements[eptr->ElementType].name);
#endif /* DEBUG_SURFACE */
			if (num_deposits >= which_deposit
					|| ++num_deposits == sizeof (DWORD) * 8)
				goto ExitCalcMinerals;
		}
		++eptr;
	} while (--j);

ExitCalcMinerals:
	return (num_deposits);
}

// Returns:
//   for whichLife==~0 : the number of nodes generated
//   for whichLife<32  : the index of the last node (no known usage exists)
// Sets the SysGenRNG to the required state first.
COUNT
GenerateMineralDeposits (SYSTEM_INFO *SysInfoPtr, COUNT whichDeposit)
{
	RandomContext_SeedRandom (SysGenRNG,
			SysInfoPtr->PlanetInfo.ScanSeed[MINERAL_SCAN]);
	return CalcMineralDeposits (SysInfoPtr, whichDeposit);
}

static COUNT
CalcLifeForms (SYSTEM_INFO *SysInfoPtr, COUNT which_life)
{
	COUNT num_life_forms;

	num_life_forms = 0;
	if (PLANSIZE (SysInfoPtr->PlanetInfo.PlanDataPtr->Type) != GAS_GIANT)
	{
#define MIN_LIFE_CHANCE 10
		SIZE life_var;

		life_var = RandomContext_Random (SysGenRNG) & 1023;
		if (life_var < SysInfoPtr->PlanetInfo.LifeChance
				|| (SysInfoPtr->PlanetInfo.LifeChance < MIN_LIFE_CHANCE
				&& life_var < MIN_LIFE_CHANCE))
		{
			BYTE num_types;

			num_types = 1 + LOBYTE (RandomContext_Random (SysGenRNG))
					% MAX_LIFE_VARIATION;
			do
			{
				BYTE index, num_creatures;
				UWORD rand_val;

				rand_val = RandomContext_Random (SysGenRNG);
				index = LOBYTE (rand_val) % NUM_CREATURE_TYPES;
				num_creatures = 1 + HIBYTE (rand_val) % 10;
				do
				{
					GenerateRandomLocation (SysInfoPtr);
					SysInfoPtr->PlanetInfo.CurType = index;

					if (num_life_forms >= which_life
							|| ++num_life_forms == sizeof (DWORD) * 8)
					{
						num_types = 1;
						break;
					}
				} while (--num_creatures);
			} while (--num_types);
		}
#ifdef DEBUG_SURFACE
		else
			log_add (log_Debug, "It's dead, Jim! (%d >= %d)", life_var,
				SysInfoPtr->PlanetInfo.LifeChance);
#endif /* DEBUG_SURFACE */
	}

	return (num_life_forms);
}

// Returns:
//   for whichLife==~0 : the number of lifeforms generated
//   for whichLife<32  : the index of the last lifeform (no known usage exists)
// Sets the SysGenRNG to the required state first.
COUNT
GenerateLifeForms (SYSTEM_INFO *SysInfoPtr, COUNT whichLife)
{
	RandomContext_SeedRandom (SysGenRNG,
			SysInfoPtr->PlanetInfo.ScanSeed[BIOLOGICAL_SCAN]);
	return CalcLifeForms (SysInfoPtr, whichLife);
}

void
GenerateRandomLocation (SYSTEM_INFO *SysInfoPtr)
{
	UWORD rand_val;

	rand_val = RandomContext_Random (SysGenRNG);
	SysInfoPtr->PlanetInfo.CurPt.x =
			(LOBYTE (rand_val) % (MAP_WIDTH - (8 << 1))) + 8;
	SysInfoPtr->PlanetInfo.CurPt.y =
			(HIBYTE (rand_val) % (MAP_HEIGHT - (8 << 1))) + 8;
}

// Returns:
//   for whichNode==~0 : the number of nodes generated
//   for whichNode<32  : the index of the last node (no known usage exists)
// Sets the SysGenRNG to the required state first.
COUNT
GenerateRandomNodes (SYSTEM_INFO *SysInfoPtr, COUNT scan, COUNT numNodes,
		COUNT type, COUNT whichNode)
{
	COUNT i;

	RandomContext_SeedRandom (SysGenRNG, SysInfoPtr->PlanetInfo.ScanSeed[scan]);

	for (i = 0; i < numNodes; ++i)
	{
		GenerateRandomLocation (SysInfoPtr);
		// CurType is irrelevant for energy nodes
		SysInfoPtr->PlanetInfo.CurType = type;
		// CurDensity is irrelevant for energy and bio nodes
		SysInfoPtr->PlanetInfo.CurDensity = 0;

		if (i >= whichNode)
			break;
	}
	
	return i;
}
