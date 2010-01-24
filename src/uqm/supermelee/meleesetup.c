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

#define MELEESETUP_INTERNAL
#include "port.h"
#include "meleesetup.h"

#include "../master.h"
#include "libs/log.h"


///////////////////////////////////////////////////////////////////////////

// Temporary
const size_t MeleeTeam_size = sizeof (MeleeTeam);

void
MeleeTeam_init (MeleeTeam *team)
{
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
		team->ships[slotI] = MELEE_NONE;
}

void
MeleeTeam_uninit (MeleeTeam *team)
{
	(void) team;
}

MeleeTeam *
MeleeTeam_new (void)
{
	MeleeTeam *result = HMalloc (sizeof (MeleeTeam));
	MeleeTeam_init (result);
	return result;
}

void
MeleeTeam_delete (MeleeTeam *team)
{
	MeleeTeam_uninit (team);
	HFree (team);
}

int
MeleeTeam_serialize (const MeleeTeam *team, uio_Stream *stream)
{
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++) {
		if (uio_putc ((int) team->ships[slotI], stream) == EOF)
			return -1;
	}
	if (uio_fwrite ((const char *) team->name, sizeof team->name, 1,
			stream) != 1)
		return -1;

	return 0;
}

int
MeleeTeam_deserialize (MeleeTeam *team, uio_Stream *stream)
{
	FleetShipIndex slotI;

	// Sanity check on the ships.
	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		int ship = uio_getc (stream);
		if (ship == EOF)
			goto err;
		team->ships[slotI] = (MeleeShip) ship;

		if (team->ships[slotI] == MELEE_NONE)
			continue;

		if (team->ships[slotI] >= NUM_MELEE_SHIPS)
		{
			log_add (log_Warning, "Invalid ship type in loaded team (index "
					"%d, ship type is %d, max valid is %d).",
					slotI, team->ships[slotI], NUM_MELEE_SHIPS - 1);
			team->ships[slotI] = MELEE_NONE;
		}
	}
	
	if (uio_fread (team->name, sizeof team->name, 1, stream) != 1)
		goto err;

	team->name[MAX_TEAM_CHARS] = '\0';

	return 0;

err:
	MeleeTeam_delete(team);
	return -1;
}

// XXX: move this to elsewhere?
COUNT
MeleeTeam_getValue (const MeleeTeam *team)
{
	COUNT total = 0;
	FleetShipIndex slotI;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		MeleeShip ship = team->ships[slotI];
		COUNT shipValue = GetShipValue (ship);
		if (shipValue == (COUNT)~0)
		{
			// Invalid ship.
			continue;
		}
		total += shipValue;
	}

	return total;
}

MeleeShip
MeleeTeam_getShip (const MeleeTeam *team, FleetShipIndex slotNr)
{
	return team->ships[slotNr];
}

void
MeleeTeam_setShip (MeleeTeam *team, FleetShipIndex slotNr, MeleeShip ship)
{
	team->ships[slotNr] = ship;
}

const MeleeShip *
MeleeTeam_getFleet (const MeleeTeam *team)
{
	return team->ships;
}

const char *
MeleeTeam_getTeamName (const MeleeTeam *team)
{
	return team->name;
}

// Returns true iff the state has actually changed.
void
MeleeTeam_setName (MeleeTeam *team, const UNICODE *name)
{
	strncpy (team->name, name, sizeof team->name - 1);
	team->name[sizeof team->name - 1] = '\0';
}

void
MeleeTeam_copy (MeleeTeam *copy, const MeleeTeam *original)
{
	*copy = *original;
}

bool
MeleeTeam_isEqual (const MeleeTeam *team1, const MeleeTeam *team2)
{
	const MeleeShip *fleet1;
	const MeleeShip *fleet2;
	FleetShipIndex slotI;

	if (strcmp (team1->name, team2->name) != 0)
		return false;

	fleet1 = team1->ships;
	fleet2 = team2->ships;

	for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
	{
		if (fleet1[slotI] != fleet2[slotI])
			return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////

MeleeSetup *
MeleeSetup_new (void)
{
	size_t teamI;
	MeleeSetup *result = HMalloc (sizeof (MeleeSetup));
	if (result == NULL)
		return NULL;

	for (teamI = 0; teamI < NUM_SIDES; teamI++)
	{
		MeleeTeam_init (&result->teams[teamI]);
		result->fleetValue[teamI] = 0;
#ifdef NETPLAY
		MeleeTeam_init (&result->confirmedTeams[teamI]);
#endif  /* NETPLAY */
	}
	return result;
}

void
MeleeSetup_delete (MeleeSetup *setup)
{
	HFree (setup);
}

// Returns true iff the state has actually changed.
bool
MeleeSetup_setShip (MeleeSetup *setup, size_t teamNr, FleetShipIndex slotNr,
		MeleeShip ship)
{
	MeleeTeam *team = &setup->teams[teamNr];
	MeleeShip oldShip = MeleeTeam_getShip (team, slotNr);

	if (ship == oldShip)
		return false;

	if (oldShip != MELEE_NONE)
		setup->fleetValue[teamNr] -= GetShipCostFromIndex (oldShip);

	MeleeTeam_setShip (team, slotNr, ship);

	if (ship != MELEE_NONE)
		setup->fleetValue[teamNr] += GetShipCostFromIndex (ship);
		
	return true;
}

MeleeShip
MeleeSetup_getShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr)
{
	return MeleeTeam_getShip (&setup->teams[teamNr], slotNr);
}

const MeleeShip *
MeleeSetup_getFleet (const MeleeSetup *setup, size_t teamNr)
{
	return MeleeTeam_getFleet (&setup->teams[teamNr]);
}

// Returns true iff the state has actually changed.
bool
MeleeSetup_setTeamName (MeleeSetup *setup, size_t teamNr,
		const UNICODE *name)
{
	MeleeTeam *team = &setup->teams[teamNr];
	const UNICODE *oldName = MeleeTeam_getTeamName (team);

	if (strcmp (oldName, name) == 0)
		return false;

	MeleeTeam_setName (team, name);
	return true;
}

const char *
MeleeSetup_getTeamName (const MeleeSetup *setup, size_t teamNr)
{
	return MeleeTeam_getTeamName (&setup->teams[teamNr]);
}

COUNT
MeleeSetup_getFleetValue (const MeleeSetup *setup, size_t teamNr)
{
	return setup->fleetValue[teamNr];
}

int
MeleeSetup_deserializeTeam (MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream)
{
	MeleeTeam *team = &setup->teams[teamNr];
	return MeleeTeam_deserialize (team, stream);
}

int
MeleeSetup_serializeTeam (const MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream)
{
	const MeleeTeam *team = &setup->teams[teamNr];
	return MeleeTeam_serialize (team, stream);
}

#ifdef NETPLAY
MeleeShip
MeleeSetup_getConfirmedShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr)
{
	return MeleeTeam_getShip (&setup->confirmedTeams[teamNr], slotNr);
}

const char *
MeleeSetup_getConfirmedTeamName (const MeleeSetup *setup, size_t teamNr)
{
	return MeleeTeam_getTeamName (&setup->confirmedTeams[teamNr]);
}

// Returns true iff the state has actually changed.
bool
MeleeSetup_setConfirmedShip (MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship)
{
	MeleeTeam *team = &setup->confirmedTeams[teamNr];
	MeleeShip oldShip = MeleeTeam_getShip (team, slotNr);

	if (ship == oldShip)
		return false;

	MeleeTeam_setShip (team, slotNr, ship);
	return true;
}

// Returns true iff the state has actually changed.
bool
MeleeSetup_setConfirmedTeamName (MeleeSetup *setup, size_t teamNr,
		const UNICODE *name)
{
	MeleeTeam *team = &setup->confirmedTeams[teamNr];
	const UNICODE *oldName = MeleeTeam_getTeamName (team);

	if (strcmp (oldName, name) == 0)
		return false;

	MeleeTeam_setName (team, name);
	return true;
}

bool
MeleeSetup_isTeamConfirmed (MeleeSetup *setup, size_t teamNr)
{
	MeleeTeam *localTeam = &setup->teams[teamNr];
	MeleeTeam *confirmedTeam = &setup->confirmedTeams[teamNr];

	return MeleeTeam_isEqual (localTeam, confirmedTeam);
}

#endif  /* NETPLAY */

///////////////////////////////////////////////////////////////////////////


