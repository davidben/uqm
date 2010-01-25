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

#ifndef MELEESETUP_H
#define MELEESETUP_H

typedef struct MeleeTeam MeleeTeam;
typedef struct MeleeSetup MeleeSetup;

#ifdef MELEESETUP_INTERNAL
#	define MELEETEAM_INTERNAL
#endif  /* MELEESETUP_INTERNAL */

#include "libs/compiler.h"

typedef COUNT FleetShipIndex;

#include "libs/uio.h"
#include "melee.h"
#include "meleeship.h"

#ifdef MELEETEAM_INTERNAL
struct MeleeTeam
{
	MeleeShip ships[MELEE_FLEET_SIZE];
	UNICODE name[MAX_TEAM_CHARS + 1 + 24];
			/* The +1 is for the terminating \0; the +24 is in case some
			 * default name in starcon.txt is unknowingly mangled. */
};
#endif  /* MELEETEAM_INTERNAL */

#ifdef MELEESETUP_INTERNAL
// When Netplay is enabled, we keep two copies of the teams: the team as we
// consider it to be locally, and last confirmed team.
// There are no separate change and confirmation messages; a confirmation
// message is a message reporting a change to the value which was specified
// by the remote side.
// When a local change is made, the current team is updated, and a message
// is sent to the other side.
// When we receive a message of a remote update:
// - if the current value is the same as the received value, we change
//   the confirmed value to the current/received value
// - if the current value is different from the received value, and
//   the current value is the same as the confirmed value, we change both
//   the current value and the confirmed value to the received value,
//   and sent a message to the remote side of our modification.
// - if the current value is different from the received value, and
//   the current value is not the same as the confirmed value, our
//   action depends on who "owns" the value:
//   - if the change is to the remote fleet, we change the current and
//     confirmed value to the received value, and send a message to the
//     remote side of our modification (as above)
//   - if the change is to the local fleet, we take no action.
//     (In this situation, a local change has been made, and a message of
//     this has already been sent to the other side, and a confirmation
//     from the other side will arrive eventually.)
// XXX: put this in docs/netplay/protocols.
// XXX: this not only works for teams, but also for other properties.
struct MeleeSetup
{
	MeleeTeam teams[NUM_SIDES];
	COUNT fleetValue[NUM_SIDES];
#ifdef NETPLAY
	MeleeTeam confirmedTeams[NUM_SIDES];
			// The least team which both sides agreed upon.
			// XXX: this may actually be deallocated when the battle starts.
#endif
};

#endif  /* MELEESETUP_INTERNAL */

extern const size_t MeleeTeam_size;

void MeleeTeam_init (MeleeTeam *team);
void MeleeTeam_uninit (MeleeTeam *team);
MeleeTeam *MeleeTeam_new (void);
void MeleeTeam_delete (MeleeTeam *team);
int MeleeTeam_serialize (const MeleeTeam *team, uio_Stream *stream);
int MeleeTeam_deserialize (MeleeTeam *team, uio_Stream *stream);
COUNT MeleeTeam_getValue (const MeleeTeam *team);
MeleeShip MeleeTeam_getShip (const MeleeTeam *team, FleetShipIndex slotNr);
void MeleeTeam_setShip (MeleeTeam *team, FleetShipIndex slotNr,
		MeleeShip ship);
const MeleeShip *MeleeTeam_getFleet (const MeleeTeam *team);
const char *MeleeTeam_getTeamName (const MeleeTeam *team);
void MeleeTeam_setName (MeleeTeam *team, const UNICODE *name);
void MeleeTeam_copy (MeleeTeam *copy, const MeleeTeam *original);
#if 0
bool MeleeTeam_isEqual (const MeleeTeam *team1, const MeleeTeam *team2);
#endif

#ifdef NETPLAY
MeleeShip MeleeSetup_getConfirmedShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr);
const char *MeleeSetup_getConfirmedTeamName (const MeleeSetup *setup,
		size_t teamNr);
bool MeleeSetup_setConfirmedShip (MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship);
bool MeleeSetup_setConfirmedTeamName (MeleeSetup *setup, size_t teamNr,
		const UNICODE *name);
#if 0
bool MeleeSetup_isTeamConfirmed (MeleeSetup *setup, size_t teamNr);
#endif
#endif  /* NETPLAY */

MeleeSetup *MeleeSetup_new (void);
void MeleeSetup_delete (MeleeSetup *setup);

bool MeleeSetup_setShip (MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship);
MeleeShip MeleeSetup_getShip (const MeleeSetup *setup, size_t teamNr,
		FleetShipIndex slotNr);
bool MeleeSetup_setFleet (MeleeSetup *setup, size_t teamNr,
		const MeleeShip *fleet);
const MeleeShip *MeleeSetup_getFleet (const MeleeSetup *setup, size_t teamNr);
bool MeleeSetup_setTeamName (MeleeSetup *setup, size_t teamNr,
		const UNICODE *name);
const char *MeleeSetup_getTeamName (const MeleeSetup *setup,
		size_t teamNr);
COUNT MeleeSetup_getFleetValue (const MeleeSetup *setup, size_t teamNr);
int MeleeSetup_deserializeTeam (MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream);
int MeleeSetup_serializeTeam (const MeleeSetup *setup, size_t teamNr,
		uio_Stream *stream);


void MeleeState_setShip (MELEE_STATE *pMS, size_t teamNr,
		FleetShipIndex slotNr, MeleeShip ship);
void MeleeState_setFleet (MELEE_STATE *pMS, size_t teamNr,
		const MeleeShip *fleet);
void MeleeState_setTeamName (MELEE_STATE *pMS, size_t teamNr,
		const UNICODE *name);
void MeleeState_setTeam (MELEE_STATE *pMS, size_t teamNr,
		const MeleeTeam *team);

#endif  /* MELEESETUP_H */

