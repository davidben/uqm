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

typedef QUEUE_HANDLE HELEMENT;

// Bits for ELEMENT_FLAGS:
#define GOOD_GUY (1 << 0)
#define BAD_GUY (1 << 1)
#define PLAYER_SHIP (1 << 2)

#define APPEARING (1 << 3)
#define DISAPPEARING (1 << 4)
#define CHANGING (1 << 5)

#define NONSOLID (1 << 6)
#define COLLISION (1 << 7)
#define IGNORE_SIMILAR (1 << 8)
#define DEFY_PHYSICS (1 << 9)

#define FINITE_LIFE (1 << 10)

#define PRE_PROCESS (1 << 11)
#define POST_PROCESS (1 << 12)

#define IGNORE_VELOCITY (1 << 13)
#define CREW_OBJECT (1 << 14)
#define BACKGROUND_OBJECT (1 << 15)


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
		PFRAME farray;
	} image;
} STATE;
typedef STATE *PSTATE;

// Any physical object in the simulation.
typedef struct element
{
	HELEMENT pred, succ;

	void (*preprocess_func) (struct element *ElementPtr);
	void (*postprocess_func) (struct element *ElementPtr);
	void (*collision_func) (struct element *ElementPtr0, PPOINT
			pPt0, struct element *ElementPtr1, PPOINT pPt1);
	void (*death_func) (struct element *ElementPtr);

	ELEMENT_FLAGS state_flags;
	COUNT life_span;
	COUNT crew_level;
	BYTE mass_points;
	BYTE turn_wait, thrust_wait;
	VELOCITY_DESC velocity;
	INTERSECT_CONTROL IntersectControl;
	COUNT PrimIndex;
	STATE current, next;

	PVOID pParent;
			// The ship this element belongs to.
	HELEMENT hTarget;
} ELEMENT;
typedef ELEMENT *PELEMENT;

#define MAX_DISPLAY_PRIMS 280
extern COUNT DisplayFreeList;
extern PRIMITIVE DisplayArray[MAX_DISPLAY_PRIMS];

#define AllocDisplayPrim() DisplayFreeList; \
								DisplayFreeList = GetSuccLink (GetPrimLinks (&DisplayArray[DisplayFreeList]))
#define FreeDisplayPrim(p) SetPrimLinks (&DisplayArray[p], END_OF_LIST, DisplayFreeList); \
								DisplayFreeList = (p)

#define STATEPTR PSTATE
#define ELEMENTPTR PELEMENT

#define GetElementStarShip(e,psd) *(psd) = (PVOID)(e)->pParent
#define SetElementStarShip(e,psd) ((e)->pParent = (PVOID)(psd))

#define blast_offset thrust_wait
#define hit_points crew_level
#define next_turn thrust_wait
#define MAX_CREW_SIZE 42
#define MAX_ENERGY_SIZE 42
#define MAX_SHIP_MASS 10
#define GRAVITY_MASS(m) ((m) > MAX_SHIP_MASS * 10)
#define GRAVITY_THRESHOLD (COUNT)255

#define WHICH_SIDE(f) (((f) & BAD_GUY) >> 1)
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
#define LockElement(h,eptr) *(eptr) = (ELEMENTPTR)LockLink (&disp_q, h)
#define UnlockElement(h) UnlockLink (&disp_q, h)
#define GetPredElement(l) _GetPredLink (l)
#define GetSuccElement(l) _GetSuccLink (l)
extern void RemoveElement (HLINK hLink);

extern void RedrawQueue (BOOLEAN clear);
extern BOOLEAN DeltaEnergy (ELEMENTPTR ElementPtr, SIZE
		energy_delta);
extern BOOLEAN DeltaCrew (ELEMENTPTR ElementPtr, SIZE
		crew_delta);

extern void PreProcessStatus (PELEMENT ShipPtr);
extern void PostProcessStatus (PELEMENT ShipPtr);

extern void load_gravity_well (BYTE selector);
extern void free_gravity_well (void);
extern void spawn_planet (void);
extern void spawn_asteroid (PELEMENT ElementPtr);
extern void animation_preprocess (PELEMENT ElementPtr);
extern void do_damage (ELEMENTPTR ElementPtr, SIZE
		damage);
extern void collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1);
extern void crew_preprocess (PELEMENT ElementPtr);
extern void crew_collision (PELEMENT ElementPtr0, PPOINT
		pPt0, PELEMENT ElementPtr1, PPOINT pPt1);
extern void AbandonShip (ELEMENTPTR ShipPtr, ELEMENTPTR
		TargetPtr, COUNT crew_loss);
extern BOOLEAN TimeSpaceMatterConflict (ELEMENTPTR
		ElementPtr);
extern COUNT PlotIntercept (ELEMENTPTR ElementPtr0,
		ELEMENTPTR ElementPtr1, COUNT max_turns, COUNT
		margin_of_error);
extern BOOLEAN LoadKernel (int argc, char *argv[]);
extern void FreeKernel (void);

extern void InitDisplayList (void);

extern void InitGalaxy (void);
extern void MoveGalaxy (VIEW_STATE view_state, 
		SIZE dx, SIZE dy);
extern void ship_preprocess (PELEMENT ElementPtr);
extern void ship_postprocess (PELEMENT
		ElementPtr);
extern void ship_death (PELEMENT ShipPtr);
extern BOOLEAN hyper_transition (PELEMENT ElementPtr);

extern BOOLEAN CalculateGravity (PELEMENT ElementPtr);
extern UWORD inertial_thrust (ELEMENTPTR ElementPtr);
extern void SetUpElement (ELEMENTPTR
		ElementPtr);

extern void BattleSong (BOOLEAN DoPlay);
extern void FreeBattleSong (void);

extern void InsertPrim (PRIM_LINKS *pLinks, COUNT primIndex, COUNT iPI);

#endif /* _ELEMENT_H */

