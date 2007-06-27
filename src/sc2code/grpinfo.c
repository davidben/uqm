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

#include "build.h"
#include "encount.h"
#include "file.h"
#include "globdata.h"
#include "intel.h"
#include "sis.h"
#include "state.h"

#include "libs/mathlib.h"
#include "libs/log.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static BYTE LastEncGroup;
		// Last encountered group, saved into state files

//#define DEBUG_GROUPS

typedef struct
{
	BYTE NumGroups;
	BYTE day_index, month_index;
	COUNT star_index, year_index;
			// day_index, month_index, year_index specify when
			//   random groups expire (if you were to leave the system
			//   by going to HSpace and stay there till such time)
			// star_index is the index of a star this group header
			//   applies to; ~0 means uninited
	DWORD GroupOffset[MAX_BATTLE_GROUPS + 1];
			// Absolute offsets of group definitions in a state file
			// Group 0 is actually a list of groups present in solarsys
			// Groups 1..max are definitions of actual battle groups
			//    containing ships composition and status

	// Each group has the following format:
	// 1 byte, RaceType (LastEncGroup in Group 0)
	// 1 byte, NumShips
	// Ships follow:
	// 1 byte, RaceType
	// 16 bytes, part of SHIP_FRAGMENT struct

} GROUP_HEADER;

static void
ReadGroupHeader (void *fp, GROUP_HEADER *pGH)
{
	sread_8   (fp, &pGH->NumGroups);
	sread_8   (fp, &pGH->day_index);
	sread_8   (fp, &pGH->month_index);
	sread_8   (fp, NULL); /* padding */
	sread_16  (fp, &pGH->star_index);
	sread_16  (fp, &pGH->year_index);
	sread_a32 (fp, pGH->GroupOffset, MAX_BATTLE_GROUPS + 1);
}

static void
WriteGroupHeader (void *fp, const GROUP_HEADER *pGH)
{
	swrite_8   (fp, pGH->NumGroups);
	swrite_8   (fp, pGH->day_index);
	swrite_8   (fp, pGH->month_index);
	swrite_8   (fp, 0); /* padding */
	swrite_16  (fp, pGH->star_index);
	swrite_16  (fp, pGH->year_index);
	swrite_a32 (fp, pGH->GroupOffset, MAX_BATTLE_GROUPS + 1);
}

static void
ReadShipFragment (void *fp, SHIP_FRAGMENT *FragPtr)
{
	BYTE tmpb;

	// Read SHIP_FRAGMENT elements
	sread_16 (fp, &FragPtr->which_side);
	sread_8  (fp, &FragPtr->captains_name_index);
	sread_8  (fp, NULL); /* padding */
	sread_16 (fp, &FragPtr->ship_flags);
	sread_8  (fp, &FragPtr->var1);
	sread_8  (fp, &FragPtr->var2);
	// XXX: reading crew as BYTE to maintain savegame compatibility
	sread_8  (fp, &tmpb);
	FragPtr->crew_level = tmpb;
	sread_8  (fp, &tmpb);
	FragPtr->max_crew = tmpb;
	sread_8  (fp, &FragPtr->energy_level);
	sread_8  (fp, &FragPtr->max_energy);
	sread_16 (fp, &FragPtr->loc.x);
	sread_16 (fp, &FragPtr->loc.y);
}

static void
WriteShipFragment (void *fp, const SHIP_FRAGMENT *FragPtr)
{
	// Write SHIP_FRAGMENT elements
	swrite_16 (fp, FragPtr->which_side);
	swrite_8  (fp, FragPtr->captains_name_index);
	swrite_8  (fp, 0); /* padding */
	swrite_16 (fp, FragPtr->ship_flags);
	swrite_8  (fp, FragPtr->var1);
	swrite_8  (fp, FragPtr->var2);
	// XXX: writing crew as BYTE to maintain savegame compatibility
	swrite_8  (fp, FragPtr->crew_level);
	swrite_8  (fp, FragPtr->max_crew);
	swrite_8  (fp, FragPtr->energy_level);
	swrite_8  (fp, FragPtr->max_energy);
	swrite_16 (fp, FragPtr->loc.x);
	swrite_16 (fp, FragPtr->loc.y);
}

void
InitGroupInfo (BOOLEAN FirstTime)
{
	void *fp;

	fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
	if (fp)
	{
		GROUP_HEADER GH;

		GH.NumGroups = 0;
		GH.star_index = (COUNT)~0;
		GH.GroupOffset[0] = 0;
		WriteGroupHeader (fp, &GH);

		CloseStateFile (fp);
	}

	if (FirstTime && (fp = OpenStateFile (DEFGRPINFO_FILE, "wb")))
	{
		// Group headers cannot start with offset 0 in
		// defined group info file, so bump it
		swrite_8 (fp, 0);

		CloseStateFile (fp);
	}
}

void
UninitGroupInfo (void)
{
	DeleteStateFile (DEFGRPINFO_FILE);
	DeleteStateFile (RANDGRPINFO_FILE);
}

void
BuildGroups (void)
{
	BYTE Index, BestIndex;
	COUNT BestPercent;
	POINT universe;
	HFLEETINFO hFleet, hNextFleet;
	BYTE HomeWorld[] =
	{
		0,                /* ARILOU_SHIP */
		0,                /* CHMMR_SHIP */
		0,                /* HUMAN_SHIP */
		ORZ_DEFINED,      /* ORZ_SHIP */
		PKUNK_DEFINED,    /* PKUNK_SHIP */
		0,                /* SHOFIXTI_SHIP */
		SPATHI_DEFINED,   /* SPATHI_SHIP */
		SUPOX_DEFINED,    /* SUPOX_SHIP */
		THRADD_DEFINED,   /* THRADDASH_SHIP */
		UTWIG_DEFINED,    /* UTWIG_SHIP */
		VUX_DEFINED,      /* VUX_SHIP */
		YEHAT_DEFINED,    /* YEHAT_SHIP */
		0,                /* MELNORME_SHIP */
		DRUUGE_DEFINED,   /* DRUUGE_SHIP */
		ILWRATH_DEFINED,  /* ILWRATH_SHIP */
		MYCON_DEFINED,    /* MYCON_SHIP */
		0,                /* SLYLANDRO_SHIP */
		UMGAH_DEFINED,    /* UMGAH_SHIP */
		0,                /* URQUAN_SHIP */
		ZOQFOT_DEFINED,   /* ZOQFOTPIK_SHIP */

		0,                /* SYREEN_SHIP */
		0,                /* BLACK_URQUAN_SHIP */
		0,                /* YEHAT_REBEL_SHIP */
	};
	BYTE EncounterPercent[] =
	{
		RACE_INTERPLANETARY_PERCENT
	};

	EncounterPercent[SLYLANDRO_SHIP] *= GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
	Index = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
	if (Index > 1 && Index < 5)
		HomeWorld[UTWIG_SHIP] = HomeWorld[SUPOX_SHIP] = 0;

	BestPercent = 0;
	universe = CurStarDescPtr->star_pt;
	for (hFleet = GetHeadLink (&GLOBAL (avail_race_q)), Index = 0;
			hFleet; hFleet = hNextFleet, ++Index)
	{
		COUNT i, encounter_radius;
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
		hNextFleet = _GetSuccLink (FleetPtr);

		if ((encounter_radius = FleetPtr->actual_strength)
				&& (i = EncounterPercent[Index]))
		{
			SIZE dx, dy;
			DWORD d_squared;
			BYTE race_enc;

			race_enc = HomeWorld[Index];
			if (race_enc && CurStarDescPtr->Index == race_enc)
			{	// In general, there are always ships at the Homeworld for
				// the races specified in HomeWorld[] array.
				BestIndex = Index;
				BestPercent = 70;
				if (race_enc == SPATHI_DEFINED || race_enc == SUPOX_DEFINED)
					BestPercent = 2;
				// Terminate the loop!
				hNextFleet = 0;

				goto FoundHome;
			}

			if (encounter_radius == (COUNT)~0)
				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
			else
				encounter_radius =
						(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;
			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
				dx = -dx;
			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
				dy = -dy;
			if ((COUNT)dx < encounter_radius
					&& (COUNT)dy < encounter_radius
					&& (d_squared = (DWORD)dx * dx + (DWORD)dy * dy) <
					(DWORD)encounter_radius * encounter_radius)
			{
				DWORD rand_val;

				// EncounterPercent is only used in practice for the Slylandro
				// Probes, for the rest of races the chance of encounter is
				// calced directly below from the distance to the Homeworld
				if (FleetPtr->actual_strength != (COUNT)~0)
				{
					i = 70 - (COUNT)((DWORD)square_root (d_squared)
							* 60L / encounter_radius);
				}

				rand_val = TFB_Random ();
				if ((int)(LOWORD (rand_val) % 100) < (int)i
						&& (BestPercent == 0
						|| (HIWORD (rand_val) % (i + BestPercent)) < i))
				{
					if (FleetPtr->actual_strength == (COUNT)~0)
					{	// The prevailing encounter chance is hereby limitted
						// to 4% for races with infinite SoI (currently, it
						// is only the Slylandro Probes)
						i = 4;
					}

					BestPercent = i;
					BestIndex = Index;
				}
			}
		}

FoundHome:
		UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	}

	if (BestPercent)
	{
		BYTE which_group, num_groups;
		BYTE EncounterMakeup[] =
		{
			RACE_ENCOUNTER_MAKEUP
		};

		which_group = 0;
		num_groups = ((COUNT)TFB_Random () % (BestPercent >> 1)) + 1;
		if (num_groups > (MAX_BATTLE_GROUPS >> 1))
			num_groups = (MAX_BATTLE_GROUPS >> 1);
		else if (num_groups < 5
				&& (Index = HomeWorld[BestIndex])
				&& CurStarDescPtr->Index == Index)
			num_groups = 5;
		do
		{
			for (Index = HINIBBLE (EncounterMakeup[BestIndex]); Index;
					--Index)
			{
				if (Index <= LONIBBLE (EncounterMakeup[BestIndex])
						|| (COUNT)TFB_Random () % 100 < 50)
					CloneShipFragment (BestIndex,
							&GLOBAL (npc_built_ship_q), 0);
			}

			PutGroupInfo (GROUPS_RANDOM, ++which_group);
			ReinitQueue (&GLOBAL (npc_built_ship_q));
		} while (--num_groups);
	}

	GetGroupInfo (GROUPS_RANDOM, GROUP_INIT_IP);
}

static void
FlushGroupInfo (GROUP_HEADER* pGH, DWORD offset, BYTE which_group, void *fp)
{
	BYTE RaceType, NumShips;
	HSHIPFRAG hStarShip;
	SHIP_FRAGMENT *FragPtr;

	if (which_group == GROUP_LIST)
	{
		QUEUE temp_q;
		HSHIPFRAG hNextShip;

		if (pGH->GroupOffset[0] == 0)
			pGH->GroupOffset[0] = LengthStateFile (fp);

		/* Weed out the dead groups first */
		temp_q = GLOBAL (npc_built_ship_q);
		SetHeadLink (&GLOBAL (npc_built_ship_q), 0);
		SetTailLink (&GLOBAL (npc_built_ship_q), 0);
		for (hStarShip = GetHeadLink (&temp_q);
				hStarShip; hStarShip = hNextShip)
		{
			COUNT crew_level;

			FragPtr = LockShipFrag (&temp_q, hStarShip);
			hNextShip = _GetSuccLink (FragPtr);
			crew_level = FragPtr->crew_level;
			which_group = GET_GROUP_ID (FragPtr);
			UnlockShipFrag (&temp_q, hStarShip);

			if (crew_level == 0)
			{	/* This group is dead -- remove it */
				if (GLOBAL (BattleGroupRef))
					PutGroupInfo (GLOBAL (BattleGroupRef), which_group);
				else
					FlushGroupInfo (pGH, GROUPS_RANDOM, which_group, fp);
				pGH->GroupOffset[which_group] = 0;
				RemoveQueue (&temp_q, hStarShip);
				FreeShipFrag (&temp_q, hStarShip);
			}
		}
		GLOBAL (npc_built_ship_q) = temp_q;

		which_group = GROUP_LIST;
	}
	else if (which_group > pGH->NumGroups)
	{	/* Group not present yet -- add it */
		pGH->NumGroups = which_group;
		pGH->GroupOffset[which_group] = LengthStateFile (fp);

		/* The first ship in a group defines the alien race */
		hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
		FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		RaceType = GET_RACE_ID (FragPtr);
		SeekStateFile (fp, pGH->GroupOffset[which_group], SEEK_SET);
		swrite_8 (fp, RaceType);
		UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
	}
	SeekStateFile (fp, offset, SEEK_SET);
	WriteGroupHeader (fp, pGH);
#ifdef DEBUG_GROUPS
	log_add (log_Debug, "1)FlushGroupInfo(%lu): WG = %u(%lu), NG = %u, "
			"SI = %u", offset, which_group, pGH->GroupOffset[which_group],
			pGH->NumGroups, pGH->star_index);
#endif /* DEBUG_GROUPS */

	NumShips = (BYTE)CountLinks (&GLOBAL (npc_built_ship_q));

	if (which_group != GROUP_LIST)
	{	/* Do not change RaceType (skip it) */
		SeekStateFile (fp, pGH->GroupOffset[which_group] + 1, SEEK_SET);
	}
	else
	{
		SeekStateFile (fp, pGH->GroupOffset[0], SEEK_SET);
		swrite_8 (fp, LastEncGroup);
	}
	swrite_8 (fp, NumShips);

	hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
	while (NumShips--)
	{
		HSHIPFRAG hNextShip;

		FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		RaceType = GET_RACE_ID (FragPtr);
		swrite_8 (fp, RaceType);

#ifdef DEBUG_GROUPS
		if (which_group == GROUP_LIST)
			log_add (log_Debug, "F) type %u, loc %u<%d, %d>, task 0x%02x:%u",
					RaceType,
					GET_GROUP_LOC (FragPtr),
					FragPtr->loc.x,
					FragPtr->loc.y,
					GET_GROUP_MISSION (FragPtr),
					GET_GROUP_DEST (FragPtr));
#endif /* DEBUG_GROUPS */
		WriteShipFragment (fp, FragPtr);
		UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		hStarShip = hNextShip;
	}
}

BOOLEAN
GetGroupInfo (DWORD offset, BYTE which_group)
{
	void *fp;

	if (offset != GROUPS_RANDOM && which_group != GROUP_LIST)
		fp = OpenStateFile (DEFGRPINFO_FILE, "r+b");
	else
		fp = OpenStateFile (RANDGRPINFO_FILE, "r+b");

	if (fp)
	{
		BYTE RaceType, NumShips;
		GROUP_HEADER GH;
		HSHIPFRAG hStarShip;
		SHIP_FRAGMENT *FragPtr;

		SeekStateFile (fp, offset, SEEK_SET);
		ReadGroupHeader (fp, &GH);
#ifdef DEBUG_GROUPS
		log_add (log_Debug, "GetGroupInfo(%lu): %u(%lu) out of %u", offset,
				which_group, GH.GroupOffset[which_group], GH.NumGroups);
#endif /* DEBUG_GROUPS */
		if (which_group == GROUP_INIT_IP)
		{
			COUNT month_index, day_index, year_index;

			ReinitQueue (&GLOBAL (npc_built_ship_q));
#ifdef DEBUG_GROUPS
			log_add (log_Debug, "%u == %u", GH.star_index,
					(COUNT)(CurStarDescPtr - star_array));
#endif /* DEBUG_GROUPS */
			day_index = GH.day_index;
			month_index = GH.month_index;
			year_index = GH.year_index;
			if (offset == GROUPS_RANDOM
					&& (GH.star_index != (COUNT)(CurStarDescPtr - star_array)
					|| !ValidateEvent (ABSOLUTE_EVENT,
					&month_index, &day_index, &year_index)))
			{
				CloseStateFile (fp);

#ifdef DEBUG_GROUPS
				if (GH.star_index == (COUNT)(CurStarDescPtr - star_array))
					log_add (log_Debug, "GetGroupInfo: battle groups out of "
							"date %u/%u/%u!", month_index, day_index,
							year_index);
#endif /* DEBUG_GROUPS */
				// Erase random groups (out of date)
				fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
				GH.NumGroups = 0;
				GH.star_index = (COUNT)~0;
				GH.GroupOffset[0] = 0;
				WriteGroupHeader (fp, &GH);
			}
			else
			{
				/* Read IP groups into npc_built_ship_q */
				for (which_group = 1; which_group <= GH.NumGroups;
						++which_group)
				{
					if (GH.GroupOffset[which_group] == 0)
						continue;

					SeekStateFile (fp, GH.GroupOffset[which_group], SEEK_SET);
					sread_8 (fp, &RaceType);
					sread_8 (fp, &NumShips);

					if (NumShips)
					{
						BYTE task, group_loc;
						DWORD rand_val;

						hStarShip = CloneShipFragment (RaceType,
								&GLOBAL (npc_built_ship_q), 0);
						FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q),
								hStarShip);
						// XXX: STARSHIP refactor; Cannot find what might be
						//   using this info. Looks unused.
						FragPtr->which_side = BAD_GUY;
						FragPtr->captains_name_index = 0;
						// XXX: CloneShipFragment() may set ship_flags in the
						//   future, and it will collide with group_counter,
						//   but it is reset a few lines below
						SET_GROUP_ID (FragPtr, which_group);

						rand_val = TFB_Random ();
						task = (BYTE)(LOBYTE (LOWORD (rand_val))
								% ON_STATION);
						if (task == FLEE)
							task = ON_STATION;
						SET_ORBIT_LOC (FragPtr, NORMALIZE_FACING (
								LOBYTE (HIWORD (rand_val))));

						group_loc = pSolarSysState->SunDesc[0].NumPlanets;
						if (group_loc == 1 && task == EXPLORE)
							task = IN_ORBIT;
						else
							group_loc = (BYTE)((
									HIBYTE (LOWORD (rand_val)) % group_loc
									) + 1);
						SET_GROUP_DEST (FragPtr, group_loc);
						rand_val = TFB_Random ();
						FragPtr->loc.x = (LOWORD (rand_val) % 10000) - 5000;
						FragPtr->loc.y = (HIWORD (rand_val) % 10000) - 5000;
						if (task == EXPLORE)
							FragPtr->group_counter =
									((COUNT)TFB_Random () % MAX_REVOLUTIONS)
									<< FACING_SHIFT;
						else
						{
							FragPtr->group_counter = 0;
							if (task == ON_STATION)
							{
								COUNT angle;
								POINT org;

								XFormIPLoc (&pSolarSysState->PlanetDesc[
										group_loc - 1].image.origin, &org,
										FALSE);
								angle = FACING_TO_ANGLE (GET_ORBIT_LOC (
										FragPtr) + 1);
								FragPtr->loc.x = org.x
										+ COSINE (angle, STATION_RADIUS);
								FragPtr->loc.y = org.y
										+ SINE (angle, STATION_RADIUS);
								group_loc = 0;
							}
						}

						SET_GROUP_MISSION (FragPtr, task);
						SET_GROUP_LOC (FragPtr, group_loc);

#ifdef DEBUG_GROUPS
						log_add (log_Debug, "battle group %u(0x%04x) strength "
								"%u, type %u, loc %u<%d, %d>, task %u",
								which_group,
								hStarShip,
								NumShips,
								RaceType,
								group_loc,
								FragPtr->loc.x,
								FragPtr->loc.y,
								task);
#endif /* DEBUG_GROUPS */
						UnlockShipFrag (&GLOBAL (npc_built_ship_q),
								hStarShip);
					}
				}

				if (offset != GROUPS_RANDOM)
					InitGroupInfo (FALSE); 	/* Wipe out random battle groups */
				else if (ValidateEvent (ABSOLUTE_EVENT, /* still fresh */
						&month_index, &day_index, &year_index))
				{
					CloseStateFile (fp);
					return (TRUE);
				}
			}
		}
		else if (GH.GroupOffset[which_group])
		{
			COUNT ShipsLeft;

			ShipsLeft = CountLinks (&GLOBAL (npc_built_ship_q));
			if (which_group == GROUP_LIST)
			{
				SeekStateFile (fp, GH.GroupOffset[0], SEEK_SET);
				sread_8 (fp, &LastEncGroup);

				if (LastEncGroup)
				{
					if (GLOBAL (BattleGroupRef))
						PutGroupInfo (GLOBAL (BattleGroupRef), LastEncGroup);
					else
						FlushGroupInfo (&GH, offset, LastEncGroup, fp);
				}
			}
			else
			{
				LastEncGroup = which_group;
				if (offset != GROUPS_RANDOM)
					PutGroupInfo (GROUPS_RANDOM, GROUP_LIST);
				else
					FlushGroupInfo (&GH, GROUPS_RANDOM, GROUP_LIST, fp);
			}
			ReinitQueue (&GLOBAL (npc_built_ship_q));

			// skip RaceType
			SeekStateFile (fp, GH.GroupOffset[which_group] + 1, SEEK_SET);
			
			sread_8 (fp, &NumShips);
			while (NumShips--)
			{
				sread_8 (fp, &RaceType);

				hStarShip = CloneShipFragment (RaceType,
						&GLOBAL (npc_built_ship_q), 0);

				FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
				ReadShipFragment (fp, FragPtr);

#ifdef DEBUG_GROUPS
				if (which_group == GROUP_LIST)
					log_add (log_Debug, "G) type %u, loc %u<%d, %d>, "
							"task 0x%02x:%u",
							RaceType,
							GET_GROUP_LOC (FragPtr),
							FragPtr->loc.x,
							FragPtr->loc.y,
							GET_GROUP_MISSION (FragPtr),
							GET_GROUP_DEST (FragPtr));
#endif /* DEBUG_GROUPS */
				if (GET_GROUP_ID (FragPtr) != LastEncGroup
						|| which_group != GROUP_LIST
						|| ShipsLeft)
				{
#ifdef DEBUG_GROUPS
					log_add (log_Debug, "\n");
#endif /* DEBUG_GROUPS */
					UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
				}
				else
				{
#ifdef DEBUG_GROUPS
					log_add (log_Debug, " -- REMOVING");
#endif /* DEBUG_GROUPS */
					UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
					RemoveQueue (&GLOBAL (npc_built_ship_q), hStarShip);
					FreeShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
				}
			}
		}

		CloseStateFile (fp);
	}

	return (GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0);
}

DWORD
PutGroupInfo (DWORD offset, BYTE which_group)
{
	void *fp;

	if (offset != GROUPS_RANDOM && which_group != GROUP_LIST)
		fp = OpenStateFile (DEFGRPINFO_FILE, "r+b");
	else
		fp = OpenStateFile (RANDGRPINFO_FILE, "r+b");

	if (fp)
	{
		GROUP_HEADER GH;

		if (offset == GROUPS_ADD_NEW)
		{
			offset = LengthStateFile (fp);
			SeekStateFile (fp, offset, SEEK_SET);
			// XXX: All important fields are inited here, and the
			//   rest are inited below
			GH.NumGroups = 0;
			GH.star_index = (COUNT)~0;
			GH.GroupOffset[0] = 0;
			WriteGroupHeader (fp, &GH);
		}

		if (which_group != GROUP_LIST)
		{
			SeekStateFile (fp, offset, SEEK_SET);
			if (which_group == GROUP_SAVE_IP)
			{
				LastEncGroup = 0;
				which_group = GROUP_LIST;
			}
		}
		ReadGroupHeader (fp, &GH);

#ifdef NEVER
		if (GetHeadLink (&GLOBAL (npc_built_ship_q))
				|| GH.GroupOffset[0] == 0)
#endif /* NEVER */
		{
			COUNT month_index, day_index, year_index;

			month_index = 0;
			day_index = 7;
			year_index = 0;
			ValidateEvent (RELATIVE_EVENT, &month_index, &day_index,
					&year_index);
			GH.day_index = (BYTE)day_index;
			GH.month_index = (BYTE)month_index;
			GH.year_index = year_index;
		}
		GH.star_index = CurStarDescPtr - star_array;
#ifdef DEBUG_GROUPS
		log_add (log_Debug, "PutGroupInfo(%lu): %u out of %u -- %u/%u/%u",
				offset, which_group, GH.NumGroups,
				GH.month_index, GH.day_index, GH.year_index);
#endif /* DEBUG_GROUPS */

		FlushGroupInfo (&GH, offset, which_group, fp);

		CloseStateFile (fp);
	}

	return (offset);
}


