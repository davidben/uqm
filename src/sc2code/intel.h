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

#ifndef _INTEL_H
#define _INTEL_H

#include "controls.h"
#include "element.h"
#include "races.h"

typedef enum
{
	PURSUE = 0,
	AVOID,
	ENTICE,
	NO_MOVEMENT
} MOVEMENT_STATE;

typedef struct
{
	PELEMENT ObjectPtr;
	COUNT facing, which_turn;
	MOVEMENT_STATE MoveState;
} EVALUATE_DESC;
typedef EVALUATE_DESC *PEVALUATE_DESC;
#define EVALUATE_DESCPTR PEVALUATE_DESC

#define MANEUVERABILITY(pi) ((pi)->ManeuverabilityIndex)
#define WEAPON_RANGE(pi) ((pi)->WeaponRange)

#define WORLD_TO_TURN(d) ((d)>>6)

#define CLOSE_RANGE_WEAPON DISPLAY_TO_WORLD (50)
#define LONG_RANGE_WEAPON DISPLAY_TO_WORLD (1000)
#define FAST_SHIP 150
#define MEDIUM_SHIP 45
#define SLOW_SHIP 25

enum
{
	ENEMY_SHIP_INDEX = 0,
	CREW_OBJECT_INDEX,
	ENEMY_WEAPON_INDEX,
	GRAVITY_MASS_INDEX,
	FIRST_EMPTY_INDEX
};

extern STARSHIPPTR CyborgDescPtr;
extern SIZE cur_player;

extern BATTLE_INPUT_STATE computer_intelligence (COUNT player,
		STARSHIPPTR StarShipPtr);
extern BATTLE_INPUT_STATE tactical_intelligence (COUNT player,
		STARSHIPPTR StarShipPtr);
extern void ship_intelligence (ELEMENTPTR ShipPtr, EVALUATE_DESCPTR
		ObjectsOfConcern, COUNT ConcernCounter);
extern BOOLEAN ship_weapons (ELEMENTPTR ShipPtr, ELEMENTPTR OtherPtr,
		COUNT margin_of_error);

extern void Pursue (ELEMENTPTR ShipPtr, EVALUATE_DESCPTR EvalDescPtr);
extern void Entice (ELEMENTPTR ShipPtr, EVALUATE_DESCPTR EvalDescPtr);
extern void Avoid (ELEMENTPTR ShipPtr, EVALUATE_DESCPTR EvalDescPtr);
extern BOOLEAN TurnShip (ELEMENTPTR ShipPtr, COUNT angle);
extern BOOLEAN ThrustShip (ELEMENTPTR ShipPtr, COUNT angle);


#define HUMAN_CONTROL (BYTE)(1 << 0)
#define CYBORG_CONTROL (BYTE)(1 << 1)
#define PSYTRON_CONTROL (BYTE)(1 << 2)
#define NETWORK_CONTROL (BYTE)(1 << 3)
#define COMPUTER_CONTROL (CYBORG_CONTROL | PSYTRON_CONTROL)

#define STANDARD_RATING (BYTE)(1 << 4)
#define GOOD_RATING (BYTE)(1 << 5)
#define AWESOME_RATING (BYTE)(1 << 6)


#endif /* _INTEL_H */


