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

//#define DEBUG

typedef struct
{
	BYTE NumGroups, day_index, month_index;
	COUNT star_index, year_index;
	DWORD GroupOffset[MAX_BATTLE_GROUPS + 1];
} GROUP_HEADER;

void
InitGroupInfo (BOOLEAN FirstTime)
{
	PVOID fp;

	fp = OpenResFile (RANDGRPINFO_FILE, "wb");
	if (fp)
	{
		GROUP_HEADER GH;

		GH.NumGroups = 0;
		GH.star_index = (COUNT)~0;
		GH.GroupOffset[0] = 0;
		WriteResFile (&GH, sizeof (GH), 1, fp);

		CloseResFile (fp);
	}

	if (FirstTime && (fp = OpenResFile (DEFGRPINFO_FILE, "wb")))
	{
		PutResFileChar (0, fp);

		CloseResFile (fp);
	}
}

void
UninitGroupInfo (void)
{
	unlink (DEFGRPINFO_FILE);
	unlink (RANDGRPINFO_FILE);
}

void
BuildGroups (void)
{
	BYTE Index, BestIndex;
	COUNT BestPercent;
	POINT universe;
	HSTARSHIP hTemplate, hNextShip;
	BYTE HomeWorld[] =
	{
		0, /* ARILOU_SHIP */
		0, /* CHMMR_SHIP */
		0, /* HUMAN_SHIP */
		ORZ_DEFINED, /* ORZ_SHIP */
		PKUNK_DEFINED, /* PKUNK_SHIP */
		0, /* SHOFIXTI_SHIP */
		SPATHI_DEFINED, /* SPATHI_SHIP */
		SUPOX_DEFINED, /* SUPOX_SHIP */
		THRADD_DEFINED, /* THRADDASH_SHIP */
		UTWIG_DEFINED, /* UTWIG_SHIP */
		VUX_DEFINED, /* VUX_SHIP */
		YEHAT_DEFINED, /* YEHAT_SHIP */
		0, /* MELNORME_SHIP */
		DRUUGE_DEFINED, /* DRUUGE_SHIP */
		ILWRATH_DEFINED,/* ILWRATH_SHIP */
		MYCON_DEFINED, /* MYCON_SHIP */
		0, /* SLYLANDRO_SHIP */
		UMGAH_DEFINED, /* UMGAH_SHIP */
		0, /* URQUAN_SHIP */
		ZOQFOT_DEFINED, /* ZOQFOTPIK_SHIP */

		0, /* SYREEN_SHIP */
		0, /* BLACK_URQUAN_SHIP */
		0, /* YEHAT_REBEL_SHIP */
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
	for (hTemplate = GetHeadLink (&GLOBAL (avail_race_q)), Index = 0;
			hTemplate; hTemplate = hNextShip, ++Index)
	{
		COUNT i, encounter_radius;
		EXTENDED_SHIP_FRAGMENTPTR TemplatePtr;

		TemplatePtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q),
				hTemplate
				);
		hNextShip = _GetSuccLink (TemplatePtr);

		if ((encounter_radius = TemplatePtr->ShipInfo.actual_strength)
				&& (i = EncounterPercent[Index]))
		{
			SIZE dx, dy;
			DWORD d_squared;

			if ((i = HomeWorld[Index]) && CurStarDescPtr->Index == (BYTE)i)
			{
				BestIndex = Index;
				BestPercent = 70;
				if (i == SPATHI_DEFINED || i == SUPOX_DEFINED)
					BestPercent = 2;
				hNextShip = 0;

				goto FoundHome;
			}

			if (encounter_radius == (COUNT)~0)
				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
			else
				encounter_radius =
						(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;
			if ((dx = universe.x - TemplatePtr->ShipInfo.loc.x) < 0)
				dx = -dx;
			if ((dy = universe.y - TemplatePtr->ShipInfo.loc.y) < 0)
				dy = -dy;
			if ((COUNT)dx < encounter_radius
					&& (COUNT)dy < encounter_radius
					&& (d_squared = (DWORD)dx * dx + (DWORD)dy * dy) <
					(DWORD)encounter_radius * encounter_radius)
			{
				DWORD rand_val;

				if (TemplatePtr->ShipInfo.actual_strength != (COUNT)~0)
				{
					i = 70 - (COUNT)((DWORD)square_root (d_squared)
							* 60L / encounter_radius);
				}

				rand_val = Random ();
				if ((int)(LOWORD (rand_val) % 100) < (int)i
						&& (BestPercent == 0
						|| (HIWORD (rand_val) % (i + BestPercent)) < i))
				{
					if (TemplatePtr->ShipInfo.actual_strength == (COUNT)~0)
						i = 4;

					BestPercent = i;
					BestIndex = Index;
				}
			}
		}

FoundHome:
		UnlockStarShip (
				&GLOBAL (avail_race_q),
				hTemplate
				);
	}

	if (BestPercent)
	{
		BYTE which_group, num_groups;
		BYTE EncounterMakeup[] =
		{
			RACE_ENCOUNTER_MAKEUP
		};

		which_group = 0;
		num_groups = ((COUNT)Random () % (BestPercent >> 1)) + 1;
		if (num_groups > (MAX_BATTLE_GROUPS >> 1))
			num_groups = (MAX_BATTLE_GROUPS >> 1);
		else if (num_groups < 5
				&& (Index = HomeWorld[BestIndex])
				&& CurStarDescPtr->Index == Index)
			num_groups = 5;
		do
		{
			for (Index = HINIBBLE (EncounterMakeup[BestIndex]); Index; --Index)
			{
				if (Index <= LONIBBLE (EncounterMakeup[BestIndex])
						|| (COUNT)Random () % 100 < 50)
					CloneShipFragment (
							BestIndex, &GLOBAL (npc_built_ship_q), 0
							);
			}

			PutGroupInfo (0L, ++which_group);
			ReinitQueue (&GLOBAL (npc_built_ship_q));
		} while (--num_groups);
	}

	GetGroupInfo (0L, (BYTE)~0);
}

static void
FlushGroupInfo (GROUP_HEADER *pGH, DWORD offset, BYTE which_group, PVOID
		fp)
{
	BYTE RaceType, NumShips;
	HSTARSHIP hStarShip;
	SHIP_FRAGMENTPTR FragPtr;

	if (which_group == 0)
	{
		if (pGH->GroupOffset[0] == 0)
			pGH->GroupOffset[0] = LengthResFile (fp);

		{
			QUEUE temp_q;
			HSTARSHIP hNextShip;

			temp_q = GLOBAL (npc_built_ship_q);
			SetHeadLink (&GLOBAL (npc_built_ship_q), 0);
			SetTailLink (&GLOBAL (npc_built_ship_q), 0);
			for (hStarShip = GetHeadLink (&temp_q);
					hStarShip; hStarShip = hNextShip)
			{
				BYTE crew_level;

				FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&temp_q, hStarShip
						);
				hNextShip = _GetSuccLink (FragPtr);
				crew_level = FragPtr->ShipInfo.crew_level;
				which_group = GET_GROUP_ID (FragPtr);
				UnlockStarShip (
						&temp_q, hStarShip
						);

				if (crew_level == 0)
				{
					if (GLOBAL (BattleGroupRef))
						PutGroupInfo (GLOBAL (BattleGroupRef), which_group);
					else
						FlushGroupInfo (pGH, 0L, which_group, fp);
					pGH->GroupOffset[which_group] = 0;
					RemoveQueue (
							&temp_q, hStarShip
							);
					FreeStarShip (
							&temp_q, hStarShip
							);
				}
			}
			GLOBAL (npc_built_ship_q) = temp_q;

			which_group = 0;
		}
	}
	else if (which_group > pGH->NumGroups)
	{
		pGH->NumGroups = which_group;
		pGH->GroupOffset[which_group] = LengthResFile (fp);

		hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
		RaceType = GET_RACE_ID (FragPtr);
		SeekResFile (fp, pGH->GroupOffset[which_group], SEEK_SET);
		WriteResFile ((PBYTE)&RaceType, sizeof (RaceType), 1, fp);
		UnlockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
	}
	SeekResFile (fp, offset, SEEK_SET);
	WriteResFile (pGH, sizeof (*pGH), 1, fp);
#ifdef DEBUG
printf ("1)FlushGroupInfo(%lu): WG = %u(%lu), NG = %u, SI = %u\n",
offset, which_group, pGH->GroupOffset[which_group], pGH->NumGroups, pGH->star_index);
#endif /* DEBUG */

	NumShips = (BYTE)CountLinks (&GLOBAL (npc_built_ship_q));

	if (which_group)
		SeekResFile (fp, pGH->GroupOffset[which_group]
				+ sizeof (RaceType), SEEK_SET);
	else
	{
		RaceType = (BYTE)cur_player;
		SeekResFile (fp, pGH->GroupOffset[0], SEEK_SET);
		WriteResFile (&RaceType, sizeof (RaceType), 1, fp);
	}
	WriteResFile (&NumShips, sizeof (NumShips), 1, fp);

	hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
	while (NumShips--)
	{
		HSTARSHIP hNextShip;
		PBYTE Ptr;

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
		hNextShip = _GetSuccLink (FragPtr);

		RaceType = GET_RACE_ID (FragPtr);
		WriteResFile ((PBYTE)&RaceType, sizeof (RaceType), 1, fp);

#ifdef DEBUG
if (which_group == 0)
printf ("F) type %u, loc %u<%d, %d>, task 0x%02x:%u\n",
RaceType,
GET_GROUP_LOC (FragPtr),
FragPtr->ShipInfo.loc.x,
FragPtr->ShipInfo.loc.y,
GET_GROUP_MISSION (FragPtr),
GET_GROUP_DEST (FragPtr));
#endif /* DEBUG */
		Ptr = (PBYTE)&FragPtr->RaceDescPtr;
		WriteResFile ((PBYTE)Ptr,
				((PBYTE)&FragPtr->ShipInfo.race_strings) - Ptr,
				1, fp);
		UnlockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
		hStarShip = hNextShip;
	}
}

BOOLEAN
GetGroupInfo (DWORD offset, BYTE which_group)
{
	PVOID fp;

	fp = OpenResFile (
			offset && which_group ? DEFGRPINFO_FILE : RANDGRPINFO_FILE,
			"r+b"
			);
	if (fp)
	{
		BYTE RaceType, NumShips;
		GROUP_HEADER GH;
		HSTARSHIP hStarShip;
		SHIP_FRAGMENTPTR FragPtr;

		SeekResFile (fp, offset, SEEK_SET);
		ReadResFile (&GH, sizeof (GH), 1, fp);
#ifdef DEBUG
printf ("GetGroupInfo(%lu): %u(%lu) out of %u\n", offset, which_group, GH.GroupOffset[which_group], GH.NumGroups);
#endif /* DEBUG */
		if (which_group == (BYTE)~0)
		{
			COUNT month_index, day_index, year_index;

			ReinitQueue (&GLOBAL (npc_built_ship_q));
#ifdef DEBUG
printf ("%u == %u\n", GH.star_index, (COUNT)(CurStarDescPtr - star_array));
#endif /* DEBUG */
			day_index = GH.day_index;
			month_index = GH.month_index;
			year_index = GH.year_index;
			if (offset == 0
					&& (GH.star_index != (COUNT)(CurStarDescPtr - star_array)
					|| !ValidateEvent (ABSOLUTE_EVENT,
					&month_index, &day_index, &year_index)))
			{
				CloseResFile (fp);

#ifdef DEBUG
if (GH.star_index == (COUNT)(CurStarDescPtr - star_array))
printf ("GetGroupInfo: battle groups out of date %u/%u/%u!\n",
month_index, day_index, year_index);
#endif /* DEBUG */
				fp = OpenResFile (RANDGRPINFO_FILE, "wb");
				GH.NumGroups = 0;
				GH.star_index = (COUNT)~0;
				GH.GroupOffset[0] = 0;
				WriteResFile (&GH, sizeof (GH), 1, fp);
			}
			else
			{
				for (which_group = 1; which_group <= GH.NumGroups; ++which_group)
				{
					if (GH.GroupOffset[which_group] == 0)
						continue;

					SeekResFile (fp, GH.GroupOffset[which_group], SEEK_SET);
					ReadResFile (&RaceType, sizeof (RaceType), 1, fp);
					ReadResFile (&NumShips, sizeof (NumShips), 1, fp);

					if (NumShips)
					{
						BYTE task, group_loc;
						DWORD rand_val;

						hStarShip = CloneShipFragment (RaceType,
								&GLOBAL (npc_built_ship_q), 0);
						FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
								&GLOBAL (npc_built_ship_q), hStarShip
								);
						OwnStarShip (FragPtr, BAD_GUY, 0);
						SET_GROUP_ID (FragPtr, which_group);

						rand_val = Random ();
						task = (BYTE)(LOBYTE (LOWORD (rand_val)) % ON_STATION);
						if (task == FLEE)
							task = ON_STATION;
						SET_ORBIT_LOC (FragPtr,
								NORMALIZE_FACING (LOBYTE (HIWORD (rand_val))));

						group_loc = pSolarSysState->SunDesc[0].NumPlanets;
						if (group_loc == 1 && task == EXPLORE)
							task = IN_ORBIT;
						else
							group_loc = (BYTE)((
									HIBYTE (LOWORD (rand_val)) % group_loc
									) + 1);
						SET_GROUP_DEST (FragPtr, group_loc);
						rand_val = Random ();
						FragPtr->ShipInfo.loc.x =
								(LOWORD (rand_val) % 10000) - 5000;
						FragPtr->ShipInfo.loc.y =
								(HIWORD (rand_val) % 10000) - 5000;
						if (task == EXPLORE)
							FragPtr->ShipInfo.group_counter =
									((COUNT)Random ()
									% MAX_REVOLUTIONS)
									<< FACING_SHIFT;
						else
						{
							FragPtr->ShipInfo.group_counter = 0;
							if (task == ON_STATION)
							{
								COUNT angle;
								POINT org;

								XFormIPLoc (
										&pSolarSysState->PlanetDesc[
												group_loc - 1
												].image.origin,
										&org,
										FALSE
										);
								angle = FACING_TO_ANGLE (
										GET_ORBIT_LOC (FragPtr) + 1
										);
								FragPtr->ShipInfo.loc.x = org.x
										+ COSINE (angle, STATION_RADIUS);
								FragPtr->ShipInfo.loc.y = org.y
										+ SINE (angle, STATION_RADIUS);
								group_loc = 0;
							}
						}

						SET_GROUP_MISSION (FragPtr, task);
						SET_GROUP_LOC (FragPtr, group_loc);

#ifdef DEBUG
printf ("battle group %u(0x%04x) strength %u, type %u, loc %u<%d, %d>, task %u\n",
which_group,
hStarShip,
NumShips,
RaceType,
group_loc,
FragPtr->ShipInfo.loc.x,
FragPtr->ShipInfo.loc.y,
task);
#endif /* DEBUG */
						UnlockStarShip (
								&GLOBAL (npc_built_ship_q), hStarShip
								);
					}
				}

				if (offset)
					InitGroupInfo (FALSE);
						/* Random battle group */
				else if (ValidateEvent (ABSOLUTE_EVENT, /* still fresh */
						&month_index, &day_index, &year_index))
				{
					CloseResFile (fp);
					return (TRUE);
				}
			}
		}
		else if (GH.GroupOffset[which_group])
		{
			BYTE BaseGroup;
			COUNT ShipsLeft;

			ShipsLeft = CountLinks (&GLOBAL (npc_built_ship_q));
			if (which_group == 0)
			{
				SeekResFile (fp, GH.GroupOffset[0], SEEK_SET);
				ReadResFile (&BaseGroup, sizeof (BaseGroup), 1, fp);

				if (BaseGroup)
				{
					if (GLOBAL (BattleGroupRef))
						PutGroupInfo (GLOBAL (BattleGroupRef), BaseGroup);
					else
						FlushGroupInfo (&GH, offset, BaseGroup, fp);
				}
			}
			else
			{
				cur_player = (SIZE)which_group;
				if (offset)
					PutGroupInfo (0L, 0);
				else
					FlushGroupInfo (&GH, 0L, 0, fp);

				BaseGroup = which_group;
			}
			ReinitQueue (&GLOBAL (npc_built_ship_q));

			SeekResFile (fp, GH.GroupOffset[which_group]
					+ sizeof (BaseGroup), SEEK_SET);

			ReadResFile (&NumShips, sizeof (NumShips), 1, fp);
			while (NumShips--)
			{
				PBYTE Ptr;

				ReadResFile ((PBYTE)&RaceType, sizeof (RaceType), 1, fp);

				hStarShip = CloneShipFragment (RaceType,
						&GLOBAL (npc_built_ship_q), 0);

				FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (npc_built_ship_q), hStarShip
						);
				Ptr = (PBYTE)&FragPtr->RaceDescPtr;
				ReadResFile ((PBYTE)Ptr,
						((PBYTE)&FragPtr->ShipInfo.race_strings) - Ptr,
						1, fp);

#ifdef DEBUG
if (which_group == 0)
printf ("G) type %u, loc %u<%d, %d>, task 0x%02x:%u\n",
RaceType,
GET_GROUP_LOC (FragPtr),
FragPtr->ShipInfo.loc.x,
FragPtr->ShipInfo.loc.y,
GET_GROUP_MISSION (FragPtr),
GET_GROUP_DEST (FragPtr));
#endif /* DEBUG */
				if (GET_GROUP_ID (FragPtr) != BaseGroup
						|| which_group
						|| ShipsLeft)
				{
#ifdef DEBUG
printf ("\n");
#endif /* DEBUG */
					if (RaceType == SHOFIXTI_SHIP
							&& which_group
							&& !GET_GAME_STATE (SHOFIXTI_RECRUITED))
					{
						BYTE captains_name_index;

						if (!GET_GAME_STATE (SHOFIXTI_KIA))
							captains_name_index =
									NAME_OFFSET + NUM_CAPTAINS_NAMES;
						else
							captains_name_index =
									NAME_OFFSET + NUM_CAPTAINS_NAMES + 1;
						OwnStarShip (FragPtr, BAD_GUY, captains_name_index);
					}
					UnlockStarShip (
							&GLOBAL (npc_built_ship_q), hStarShip
							);
				}
				else
				{
#ifdef DEBUG
printf (" -- REMOVING\n");
#endif /* DEBUG */
					UnlockStarShip (
							&GLOBAL (npc_built_ship_q), hStarShip
							);
					RemoveQueue (
							&GLOBAL (npc_built_ship_q), hStarShip
							);
					FreeStarShip (
							&GLOBAL (npc_built_ship_q), hStarShip
							);
				}
			}
		}

		CloseResFile (fp);
	}

	return (GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0);
}

DWORD
PutGroupInfo (DWORD offset, BYTE which_group)
{
	PVOID fp;

	fp = OpenResFile (
			offset && which_group ? DEFGRPINFO_FILE : RANDGRPINFO_FILE,
			"r+b"
			);
	if (fp)
	{
		GROUP_HEADER GH;

		if (offset == (DWORD) ~0L)
		{
			offset = LengthResFile (fp);
			GH.NumGroups = 0;
			GH.GroupOffset[0] = 0;
			SeekResFile (fp, offset, SEEK_SET);
			WriteResFile (&GH, sizeof (GH), 1, fp);
		}

		if (which_group)
		{
			SeekResFile (fp, offset, SEEK_SET);
			if (which_group == (BYTE)~0)
			{
				cur_player = 0;
				which_group = 0;
			}
		}
		ReadResFile (&GH, sizeof (GH), 1, fp);

#ifdef NEVER
		if (GetHeadLink (&GLOBAL (npc_built_ship_q))
				|| GH.GroupOffset[0] == 0)
#endif /* NEVER */
		{
			COUNT month_index, day_index, year_index;

			month_index = 0;
			day_index = 7;
			year_index = 0;
			ValidateEvent (RELATIVE_EVENT, &month_index, &day_index, &year_index);
			GH.day_index = (BYTE)day_index;
			GH.month_index = (BYTE)month_index;
			GH.year_index = year_index;
		}
		GH.star_index = CurStarDescPtr - star_array;
#ifdef DEBUG
printf ("PutGroupInfo(%lu): %u out of %u -- %u/%u/%u\n",
offset, which_group, GH.NumGroups,
GH.month_index, GH.day_index, GH.year_index);
#endif /* DEBUG */

		FlushGroupInfo (&GH, offset, which_group, fp);

		CloseResFile (fp);
	}

	return (offset);
}

