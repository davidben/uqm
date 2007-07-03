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

#ifndef _ELEMENT_H
#define _ELEMENT_H

#include "battle.h"
#include "displist.h"
#include "units.h"
#include "velocity.h"
#include "libs/gfxlib.h"

#define BATTLE_FRAME_RATE (ONE_SECOND / 24)

#define SHIP_INFO_HEIGHT 65
#define CAPTAIN_XOFFS 4
#define CAPTAIN_YOFFS (SHIP_INFO_HEIGHT + 4)
#define SHIP_STATUS_HEIGHT (STATUS_HEIGHT >> 1)
#define BAD_GUY_YOFFS 0
#define GOOD_GUY_YOFFS SHIP_STATUS_HEIGHT
#define STARCON_TEXT_HEIGHT 7
#define TINY_TEXT_HEIGHT 9

#define BATTLE_CREW_X 10
#define BATTLE_CREW_Y (64 - SAFE_Y)

#define NORMAL_LIFE 1

typedef HLINK HELEMENT;

// Bits for ELEMENT_FLAGS:
#define GOOD_GUY (1 << 0)
#define BAD_GUY (1 << 1)
#define PLAYER_SHIP (1 << 2)
		// The ELEMENT is a player controlable ship, and not some bullet,
		// crew, asteroid, fighter, etc. This does not mean that the ship
		// is actually controlled by a human; it may be a computer.

#define APPEARING (1 << 3)
#define DISAPPEARING (1 << 4)
#define CHANGING (1 << 5)
		// The element's graphical representation has changed.

#define NONSOLID (1 << 6)
#define COLLISION (1 << 7)
#define IGNORE_SIMILAR (1 << 8)
#define DEFY_PHYSICS (1 << 9)

#define FINITE_LIFE (1 << 10)

#define PRE_PROCESS (1 << 11)
		// PreProcess() is to be called for the ELEMENT.
#define POST_PROCESS (1 << 12)

#define IGNORE_VELOCITY (1 << 13)
#define CREW_OBJECT (1 << 14)
#define BACKGROUND_OBJECT (1 << 15)
		// The BACKGROUND_OBJECT flag existed originally but wasn't used.
		// It can now be used for objects that never influence the state
		// of other elements; elements that have this flag set are not
		// included in the checksum used for netplay games.
		// It can be used for graphical mods that don't impede netplay.


#define HYPERJUMP_LIFE 15

#define NUM_EXPLOSION_FRAMES 12

#define GAME_SOUND_PRIORITY 2

typedef enum
{
	VIEW_STABLE,
	VIEW_SCROLL,
	VIEW_CHANGE
} VIEW_STATE;

typedef UWORD ELEMENT_FLAGS;

#define NO_PRIM NUM_PRIMS

typedef struct state
{
	POINT location;
	struct
	{
		FRAME frame;
		FRAME *farray;
	} image;
} STATE;


typedef struct element ELEMENT;

typedef void (CollisionFunc) (ELEMENT *ElementPtr0, POINT *pPt0,
			ELEMENT *ElementPtr1, POINT *pPt1);

// Any physical object in the simulation.
struct element
{
	// LINK elements; must be first
	HELEMENT pred, succ;

	void (*preprocess_func) (struct element *ElementPtr);
	void (*postprocess_func) (struct element *ElementPtr);
	CollisionFunc *collision_func;
	void (*death_func) (struct element *ElementPtr);

	ELEMENT_FLAGS state_flags;
	COUNT life_span;
	COUNT crew_level;
	BYTE mass_points;
			/* Also: system loc for IP flagship */
	BYTE turn_wait;
			/* Also: group_id for IP groups */
	BYTE thrust_wait;
	VELOCITY_DESC velocity;
	INTERSECT_CONTROL IntersectControl;
	COUNT PrimIndex;
	STATE current, next;

	void *pParent;
			// The ship this element belongs to.
	HELEMENT hTarget;
};

#define MAX_DISPLAY_PRIMS 280
extern COUNT DisplayFreeList;
extern PRIMITIVE DisplayArray[MAX_DISPLAY_PRIMS];

#define AllocDisplayPrim() DisplayFreeList; \
								DisplayFreeList = GetSuccLink (GetPrimLinks (&DisplayArray[DisplayFreeList]))
#define FreeDisplayPrim(p) SetPrimLinks (&DisplayArray[p], END_OF_LIST, DisplayFreeList); \
								DisplayFreeList = (p)

#define GetElementStarShip(e,ppsd) do { *(ppsd) = (e)->pParent; } while (0)
#define SetElementStarShip(e,psd)  do { (e)->pParent = psd; } while (0)

// XXX: Would be nice to clean these up!
//   Should make them into a union for now.
#define blast_offset thrust_wait
#define hit_points crew_level
#define next_turn thrust_wait

#define MAX_CREW_SIZE 42
#define MAX_ENERGY_SIZE 42
#define MAX_SHIP_MASS 10
#define GRAVITY_MASS(m) ((m) > MAX_SHIP_MASS * 10)
#define GRAVITY_THRESHOLD (COUNT)255

static inline BYTE
ElementFlagsSide (ELEMENT_FLAGS flags)
{
	return (BYTE) ((flags & BAD_GUY) >> 1);
}
#define WHICH_SIDE(flags) ElementFlagsSide (flags)

#define OBJECT_CLOAKED(eptr) \
		(GetPrimType (&GLOBAL (DisplayArray[(eptr)->PrimIndex])) >= NUM_PRIMS \
		|| (GetPrimType (&GLOBAL (DisplayArray[(eptr)->PrimIndex])) == STAMPFILL_PRIM \
		&& GetPrimColor (&GLOBAL (DisplayArray[(eptr)->PrimIndex])) == BLACK_COLOR))
#define UNDEFINED_LEVEL 0

extern HELEMENT AllocElement (void);
extern void FreeElement (HELEMENT hElement);
#define PutElement(h) PutQueue (&disp_q, h)
#define InsertElement(h,i) InsertQueue (&disp_q, h, i)
#define GetHeadElement() GetHeadLink (&disp_q)
#define GetTailElement() GetTailLink (&disp_q)
#define LockElement(h,ppe) (*(ppe) = (ELEMENT*)LockLink (&disp_q, h))
#define UnlockElement(h) UnlockLink (&disp_q, h)
#define GetPredElement(l) _GetPredLink (l)
#define GetSuccElement(l) _GetSuccLink (l)
extern void RemoveElement (HLINK hLink);

extern void RedrawQueue (BOOLEAN clear);
extern BOOLEAN DeltaEnergy (ELEMENT *ElementPtr, SIZE energy_delta);
extern BOOLEAN DeltaCrew (ELEMENT *ElementPtr, SIZE crew_delta);

extern void PreProcessStatus (ELEMENT *ShipPtr);
extern void PostProcessStatus (ELEMENT *ShipPtr);

extern void load_gravity_well (BYTE selector);
extern void free_gravity_well (void);
extern void spawn_planet (void);
extern void spawn_asteroid (ELEMENT *ElementPtr);
extern void animation_preprocess (ELEMENT *ElementPtr);
extern void do_damage (ELEMENT *ElementPtr, SIZE damage);
extern void collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1);
extern void crew_preprocess (ELEMENT *ElementPtr);
extern void crew_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1);
extern void AbandonShip (ELEMENT *ShipPtr, ELEMENT *TargetPtr,
		COUNT crew_loss);
extern BOOLEAN TimeSpaceMatterConflict (ELEMENT *ElementPtr);
extern COUNT PlotIntercept (ELEMENT *ElementPtr0,
		ELEMENT *ElementPtr1, COUNT max_turns, COUNT margin_of_error);
extern BOOLEAN LoadKernel (int argc, char *argv[]);
extern void FreeKernel (void);

extern void InitDisplayList (void);

extern void InitGalaxy (void);
extern void MoveGalaxy (VIEW_STATE view_state, SIZE dx, SIZE dy);
extern void ship_preprocess (ELEMENT *ElementPtr);
extern void ship_postprocess (ELEMENT *ElementPtr);
extern void ship_death (ELEMENT *ShipPtr);
extern BOOLEAN hyper_transition (ELEMENT *ElementPtr);

extern BOOLEAN CalculateGravity (ELEMENT *ElementPtr);
extern UWORD inertial_thrust (ELEMENT *ElementPtr);
extern void SetUpElement (ELEMENT *ElementPtr);

extern void BattleSong (BOOLEAN DoPlay);
extern void FreeBattleSong (void);

extern void InsertPrim (PRIM_LINKS *pLinks, COUNT primIndex, COUNT iPI);

#endif /* _ELEMENT_H */

