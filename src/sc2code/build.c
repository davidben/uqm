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

HSTARSHIP
Build (PQUEUE pQueue, DWORD RaceResIndex, COUNT which_player, BYTE
		captains_name_index)
{
	HSTARSHIP hNewShip;

	if ((hNewShip = AllocStarShip (pQueue)) != 0)
	{
		STARSHIPPTR StarShipPtr;

		StarShipPtr = LockStarShip (pQueue, hNewShip);
		memset (StarShipPtr, 0, GetLinkSize (pQueue));

		StarShipPtr->RaceResIndex = RaceResIndex;
		OwnStarShip (StarShipPtr, which_player, captains_name_index);

		UnlockStarShip (pQueue, hNewShip);
		PutQueue (pQueue, hNewShip);
	}

	return (hNewShip);
}

HSTARSHIP
GetStarShipFromIndex (PQUEUE pShipQ, COUNT Index)
{
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (pShipQ);
			Index > 0 && hStarShip; hStarShip = hNextShip, --Index)
	{
		STARSHIPPTR StarShipPtr;

		StarShipPtr = LockStarShip (pShipQ, hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockStarShip (pShipQ, hStarShip);
	}

	return (hStarShip);
}

COUNT
ActivateStarShip (COUNT which_ship, SIZE state)
{
	HSTARSHIP hStarShip, hNextShip;

	hStarShip = GetStarShipFromIndex (
			&GLOBAL (avail_race_q), which_ship
			);
	if (hStarShip)
	{
		switch (state)
		{
			case SPHERE_TRACKING:
			case SPHERE_KNOWN:
			{
				EXTENDED_SHIP_FRAGMENTPTR StarShipPtr;

				StarShipPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (avail_race_q), hStarShip
						);
				if (state == SPHERE_KNOWN)
					which_ship = StarShipPtr->ShipInfo.known_strength;
				else if (StarShipPtr->ShipInfo.actual_strength == 0)
				{
					if (!(StarShipPtr->ShipInfo.ship_flags
							& (GOOD_GUY | BAD_GUY)))
						which_ship = 0;
				}
				else if (StarShipPtr->ShipInfo.known_strength == 0
						&& StarShipPtr->ShipInfo.actual_strength != (COUNT)~0)
				{
					StarShipPtr->ShipInfo.known_strength = 1;
					StarShipPtr->ShipInfo.known_loc =
							StarShipPtr->ShipInfo.loc;
				}
				UnlockStarShip (
						&GLOBAL (avail_race_q), hStarShip
						);
				return (which_ship);
			}
			case ESCORT_WORTH:
				which_ship = 0;
			case ESCORTING_FLAGSHIP:
			{
				COUNT ShipCost[] =
				{
					RACE_SHIP_COST
				};

				for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
						hStarShip; hStarShip = hNextShip)
				{
					BYTE ship_type;
					SHIP_FRAGMENTPTR StarShipPtr;

					StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
							&GLOBAL (built_ship_q), hStarShip
							);
					hNextShip = _GetSuccLink (StarShipPtr);
					if (state == ESCORT_WORTH)
						which_ship += ShipCost[GET_RACE_ID (StarShipPtr)];
					else
						ship_type = GET_RACE_ID (StarShipPtr);
					UnlockStarShip (
							&GLOBAL (built_ship_q), hStarShip
							);

					if (state != ESCORT_WORTH
							&& (COUNT)ship_type == which_ship)
						return (1);
				}

				return (state == ESCORTING_FLAGSHIP ? 0 : which_ship);
			}
			case FEASIBILITY_STUDY:
				return (MAX_BUILT_SHIPS
						- CountLinks (&GLOBAL (built_ship_q)));
			default:
			{
				SHIP_FRAGMENTPTR StarShipPtr;

				if (state <= 0)
				{
					StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
							&GLOBAL (avail_race_q), hStarShip
							);
					if (state == CHECK_ALLIANCE)
					{
						state = StarShipPtr->ShipInfo.ship_flags
								& (GOOD_GUY | BAD_GUY);
						UnlockStarShip (
								&GLOBAL (avail_race_q), hStarShip
								);
						return ((COUNT)state);
					}
					else if (StarShipPtr->ShipInfo.ship_flags
							& (GOOD_GUY | BAD_GUY))
					{
						StarShipPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
						if (state == 0)
							StarShipPtr->ShipInfo.ship_flags |= GOOD_GUY;
						else
						{
							StarShipPtr->ShipInfo.ship_flags |= BAD_GUY;
							if (which_ship == ORZ_SHIP)
							{
								BOOLEAN ShipRemoved;

								ShipRemoved = FALSE;
								for (hStarShip = GetHeadLink (
										&GLOBAL (built_ship_q
										)); hStarShip; hStarShip = hNextShip)
								{
									BOOLEAN RemoveShip;
									SHIP_FRAGMENTPTR StarShipPtr2;

									StarShipPtr2 = (SHIP_FRAGMENTPTR)LockStarShip (
											&GLOBAL (built_ship_q),
											hStarShip
											);
									hNextShip = _GetSuccLink (StarShipPtr2);
									RemoveShip = (BOOLEAN)(
											GET_RACE_ID (StarShipPtr2) == ORZ_SHIP
											);
									UnlockStarShip (
											&GLOBAL (built_ship_q),
											hStarShip
											);

									if (RemoveShip)
									{
										ShipRemoved = TRUE;

										RemoveQueue (
												&GLOBAL (built_ship_q),
												hStarShip
												);
										FreeStarShip (
												&GLOBAL (built_ship_q),
												hStarShip
												);
									}
								}
								
								if (ShipRemoved)
								{
									SetSemaphore (GraphicsSem);
									DeltaSISGauges (UNDEFINED_DELTA,
											UNDEFINED_DELTA, UNDEFINED_DELTA);
									ClearSemaphore (GraphicsSem);
								}
							}
						}
					}
					UnlockStarShip (
							&GLOBAL (avail_race_q), hStarShip
							);
				}
				else
				{
					BYTE which_window;
						COUNT i;

					which_window = 0;
					for
						(
								i = 0;
								
								i < (COUNT)state
							&&
								(
										hStarShip = CloneShipFragment
										(
												(COUNT)which_ship,
												(PQUEUE)(&GLOBAL (built_ship_q)),
												(BYTE)
												(
														(
																which_ship == SPATHI_SHIP &&
																								GET_GAME_STATE (FOUND_PLUTO_SPATHI)
														) == 1 ? 1 : 0
												)
										)
								);

							i++
						)
					{
						HSTARSHIP hOldShip;

						RemoveQueue (
								&GLOBAL (built_ship_q),
								hStarShip
								);

						while ((hOldShip = GetStarShipFromIndex (
								&GLOBAL (built_ship_q),
								which_window++
								)))
						{
							BYTE win_loc;

							StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
									&GLOBAL (built_ship_q), hOldShip
									);
							win_loc = GET_GROUP_LOC (StarShipPtr);
							UnlockStarShip (
									&GLOBAL (built_ship_q), hOldShip
									);
							if (which_window <= win_loc)
								break;
						}

						StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
								&GLOBAL (built_ship_q), hStarShip
								);
						SET_GROUP_LOC (StarShipPtr, which_window - 1);
						if (which_ship == SPATHI_SHIP
								&& GET_GAME_STATE (FOUND_PLUTO_SPATHI) == 1)
						{
							OwnStarShip (StarShipPtr,
									GOOD_GUY,
									NAME_OFFSET + NUM_CAPTAINS_NAMES);
						}
						UnlockStarShip (
								&GLOBAL (built_ship_q), hStarShip
								);

						InsertQueue (
								&GLOBAL (built_ship_q),
								hStarShip, hOldShip
								);
					}

					SetSemaphore (GraphicsSem);
					DeltaSISGauges (UNDEFINED_DELTA,
							UNDEFINED_DELTA, UNDEFINED_DELTA);
					ClearSemaphore (GraphicsSem);
					return (i);
				}
				break;
			}
		}

		return (1);
	}

	return (0);
}

COUNT
GetIndexFromStarShip (PQUEUE pShipQ, HSTARSHIP hStarShip)
{
	COUNT Index;

	Index = 0;
	while (hStarShip != GetHeadLink (pShipQ))
	{
		HSTARSHIP hNextShip;
		STARSHIPPTR StarShipPtr;

		StarShipPtr = LockStarShip (pShipQ, hStarShip);
		hNextShip = _GetPredLink (StarShipPtr);
		UnlockStarShip (pShipQ, hStarShip);

		hStarShip = hNextShip;
		++Index;
	}

	return (Index);
}

BYTE
NameCaptain (PQUEUE pQueue, STARSHIPPTR StarShipPtr)
{
	BYTE name_index;
	HSTARSHIP hStarShip;

	do
	{
		HSTARSHIP hNextShip;

		name_index = PickCaptainName ();
		for (hStarShip = GetHeadLink (pQueue); hStarShip; hStarShip = hNextShip)
		{
			STARSHIPPTR TestShipPtr;

			TestShipPtr = LockStarShip (pQueue, hStarShip);
			hNextShip = _GetSuccLink (TestShipPtr);
			if (TestShipPtr->RaceResIndex == StarShipPtr->RaceResIndex)
			{
				BOOLEAN SameName;

				if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
					SameName = (BOOLEAN)(
							name_index == TestShipPtr->captains_name_index
							);
				else
					SameName = (BOOLEAN)(
							name_index == StarShipCaptain (TestShipPtr)
							);

				if (SameName)
				{
					UnlockStarShip (pQueue, hStarShip);
					break;
				}
			}
			UnlockStarShip (pQueue, hStarShip);
		}
	} while (hStarShip);

	return (name_index);
}

HSTARSHIP
CloneShipFragment (COUNT shipIndex, PQUEUE pDstQueue, BYTE crew_level)
{
	HSTARSHIP hStarShip, hBuiltShip;
	SHIP_FRAGMENTPTR TemplatePtr;

	if ((hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q),
			shipIndex)) == 0)
		return (0);

	TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hStarShip
			);
	hBuiltShip =
			Build (pDstQueue,
			TemplatePtr->RaceResIndex,
			TemplatePtr->ShipInfo.ship_flags & (GOOD_GUY | BAD_GUY),
			(BYTE)(shipIndex == SAMATRA_SHIP ?
					0 : NameCaptain (pDstQueue, (STARSHIPPTR)TemplatePtr)));
	if (hBuiltShip)
	{
		SHIP_FRAGMENTPTR ShipFragPtr;

		ShipFragPtr = (SHIP_FRAGMENTPTR)LockStarShip (pDstQueue, hBuiltShip);
		ShipFragPtr->ShipInfo = TemplatePtr->ShipInfo;
		if (crew_level)
			ShipFragPtr->ShipInfo.crew_level = crew_level;
		ShipFragPtr->ShipInfo.energy_level = 0;
		ShipFragPtr->ShipInfo.ship_flags = 0;
		ShipFragPtr->ShipInfo.var1 = ShipFragPtr->ShipInfo.var2 = 0;
		ShipFragPtr->ShipInfo.loc.x = ShipFragPtr->ShipInfo.loc.y = 0;
		SET_RACE_ID (ShipFragPtr, (BYTE)shipIndex);
		UnlockStarShip (pDstQueue, hBuiltShip);
	}
	UnlockStarShip (
			&GLOBAL (avail_race_q), hStarShip
			);

	return (hBuiltShip);
}

