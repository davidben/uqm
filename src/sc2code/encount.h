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

#ifndef _ENCOUNT_H
#define _ENCOUNT_H

#include "displist.h"
#include "libs/gfxlib.h"
#include "planets/planets.h"
#include "races.h"


typedef QUEUE_HANDLE HENCOUNTER;

#define MAX_HYPER_SHIPS 7
#define ONE_SHOT_ENCOUNTER (1 << 7)
#define ENCOUNTER_REFORMING (1 << 6)

typedef struct
{
	POINT star_pt;
	BYTE Type, Index;
	SHIP_INFO ShipList[MAX_HYPER_SHIPS];
} EXTENDED_STAR_DESC;
typedef EXTENDED_STAR_DESC *PEXTENDED_STAR_DESC;

typedef struct
{
	HENCOUNTER pred, succ;

	HELEMENT hElement;

	SIZE transition_state;
	POINT origin;
	COUNT radius;

	EXTENDED_STAR_DESC SD;

	SDWORD log_x, log_y;
} ENCOUNTER;
typedef ENCOUNTER *PENCOUNTER;

#define AllocEncounter() AllocLink (&GLOBAL (encounter_q))
#define PutEncounter(h) PutQueue (&GLOBAL (encounter_q), h)
#define InsertEncounter(h,i) InsertQueue (&GLOBAL (encounter_q), h, i)
#define GetHeadEncounter() GetHeadLink (&GLOBAL (encounter_q))
#define GetTailEncounter() GetTailLink (&GLOBAL (encounter_q))
#define LockEncounter(h,eptr) *(eptr) = (ENCOUNTERPTR)LockLink (&GLOBAL (encounter_q), h)
#define UnlockEncounter(h) UnlockLink (&GLOBAL (encounter_q), h)
#define RemoveEncounter(h) RemoveQueue (&GLOBAL (encounter_q), h)
#define FreeEncounter(h) FreeLink (&GLOBAL (encounter_q), h)
#define GetPredEncounter(l) _GetPredLink (l)
#define GetSuccEncounter(l) _GetSuccLink (l)

#define ENCOUNTERPTR PENCOUNTER

enum
{
	SOL_DEFINED = 1,
	SHOFIXTI_DEFINED,
	MAIDENS_DEFINED,
	START_COLONY_DEFINED,
	SPATHI_DEFINED,
	ZOQFOT_DEFINED,

	MELNORME0_DEFINED,
	MELNORME1_DEFINED,
	MELNORME2_DEFINED,
	MELNORME3_DEFINED,
	MELNORME4_DEFINED,
	MELNORME5_DEFINED,
	MELNORME6_DEFINED,
	MELNORME7_DEFINED,
	MELNORME8_DEFINED,

	TALKING_PET_DEFINED,
	CHMMR_DEFINED,
	SYREEN_DEFINED,
	BURVIXESE_DEFINED,
	SLYLANDRO_DEFINED,
	DRUUGE_DEFINED,
	BOMB_DEFINED,
	AQUA_HELIX_DEFINED,
	SUN_DEVICE_DEFINED,
	TAALO_PROTECTOR_DEFINED,
	SHIP_VAULT_DEFINED,
	URQUAN_WRECK_DEFINED,
	VUX_BEAST_DEFINED,
	SAMATRA_DEFINED,
	ZOQ_SCOUT_DEFINED,
	MYCON_DEFINED,
	EGG_CASE0_DEFINED,
	EGG_CASE1_DEFINED,
	EGG_CASE2_DEFINED,
	PKUNK_DEFINED,
	UTWIG_DEFINED,
	SUPOX_DEFINED,
	YEHAT_DEFINED,
	VUX_DEFINED,
	ORZ_DEFINED,
	THRADD_DEFINED,
	RAINBOW_DEFINED,
	ILWRATH_DEFINED,
	ANDROSYNTH_DEFINED,
	MYCON_TRAP_DEFINED
};

#define UMGAH_DEFINED TALKING_PET_DEFINED

#define TEXT_X_OFFS 1
#define TEXT_Y_OFFS 1
#define SIS_TEXT_WIDTH (SIS_SCREEN_WIDTH - (TEXT_X_OFFS << 1))

extern STAR_DESCPTR CurStarDescPtr;
extern STAR_DESCPTR star_array;

#define NUM_SOLAR_SYSTEMS 502

extern STAR_DESCPTR FindStar (STAR_DESCPTR pLastStar, PPOINT puniverse,
		SIZE xbounds, SIZE ybounds);

extern void GetClusterName (STAR_DESCPTR pSD, UNICODE buf[]);

enum
{
	HAIL = 0,
	ATTACK
};


extern void EncounterBattle (void);
extern void BuildBattle (COUNT which_player);
extern COUNT InitEncounter (void);
extern COUNT UninitEncounter (void);
extern COUNT InitCommunication (RESOURCE which_comm);
extern void RaceCommunication (void);

extern void GenerateSOL (BYTE control);
extern void GenerateShofixti (BYTE control);
extern void GenerateColony (BYTE control);
extern void GenerateSpathi (BYTE control);
extern void GenerateZoqFotPik (BYTE control);
extern void GenerateMelnorme (BYTE control);
extern void GenerateTalkingPet (BYTE control);
extern void GenerateChmmr (BYTE control);
extern void GenerateSyreen (BYTE control);
extern void GenerateBurvixes (BYTE control);
extern void GenerateSlylandro (BYTE control);
extern void GenerateDruuge (BYTE control);
extern void GenerateUtwig (BYTE control);
extern void GenerateThradd (BYTE control);
extern void GenerateMycon (BYTE control);
extern void GenerateOrz (BYTE control);
extern void GenerateShipVault (BYTE control);
extern void GenerateUrquanWreck (BYTE control);
extern void GenerateVUX (BYTE control);
extern void GenerateSamatra (BYTE control);
extern void GenerateYehat (BYTE control);
extern void GeneratePkunk (BYTE control);
extern void GenerateSupox (BYTE control);
extern void GenerateRainbow (BYTE control);
extern void GenerateIlwrath (BYTE control);

extern SIZE EncounterRace;
extern BYTE EncounterGroup;

#endif /* _ENCOUNT_H */

