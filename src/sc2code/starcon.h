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

#ifndef _STARCON_H
#define _STARCON_H

#define MAX_PLAYERS 4
#define NUM_PLAYERS 2
#define NUM_SIDES 2

extern int ScreenWidth;
extern int ScreenHeight;

#define SCREEN_WIDTH ScreenWidth
#define SCREEN_HEIGHT ScreenHeight
#define SAFE_X 0
		/* Left and right screen margin to be left unused */
#define SAFE_Y 0
		/* Top and bottom screen margin to be left unused */
#define SIS_ORG_X (7 + SAFE_X)
#define SIS_ORG_Y (10 + SAFE_Y)
#define STATUS_WIDTH 64
#define STATUS_HEIGHT (SCREEN_HEIGHT - (SAFE_Y * 2))
#define SPACE_WIDTH (SCREEN_WIDTH - STATUS_WIDTH - (SAFE_X * 2))
#define SPACE_HEIGHT (SCREEN_HEIGHT - (SAFE_Y * 2))
#define SIS_SCREEN_WIDTH (SPACE_WIDTH - 14)
#define SIS_SCREEN_HEIGHT (SPACE_HEIGHT - 13)

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif
#include "../port.h"
#include <stdlib.h>
#include <math.h>
#include "gfxlib.h"
#include "inplib.h"
#include "timelib.h"
#include "sndlib.h"
#include "strlib.h"
#include "mathlib.h"
#include "reslib.h"
#include "threadlib.h"
#include "restypes.h"
#include "resinst.h"
#include "displist.h"
#include "nameref.h"
#include "units.h"
#include "velocity.h"
#include "element.h"
#include "races.h"
#include "weapon.h"
#include "collide.h"
#include "build.h"
#include "intel.h"
#include "sounds.h"
#include "demo.h"
#include "starbase.h"
#include "sis.h"
#include "state.h"
#include "globdata.h"
#include "encount.h"

#define HUMAN_CONTROL (BYTE)(1 << 0)
#define CYBORG_CONTROL (BYTE)(1 << 1)
#define PSYTRON_CONTROL (BYTE)(1 << 2)
#define COMPUTER_CONTROL (CYBORG_CONTROL | PSYTRON_CONTROL)

#define STANDARD_RATING (BYTE)(1 << 3)
#define GOOD_RATING (BYTE)(1 << 4)
#define AWESOME_RATING (BYTE)(1 << 5)

#define SELECT_RATE 20
#define INVALID_INPUT ((INPUT_STATE)0xFF)
#define INPUT_TIMED_OUT ((INPUT_STATE)0)

#define BLACK_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define LTGRAY_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define DKGRAY_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define WHITE_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)

#define NORMAL_ILLUMINATED_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define NORMAL_SHADOWED_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define HIGHLIGHT_ILLUMINATED_COLOR    BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)
#define HIGHLIGHT_SHADOWED_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define MENU_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define MENU_FOREGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define MENU_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define MENU_HIGHLIGHT_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)

#define STATUS_ILLUMINATED_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define STATUS_SHADOWED_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define STATUS_SHAPE_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define STATUS_SHAPE_OUTLINE_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)

#define CONTROL_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)

#define ALLIANCE_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define HIERARCHY_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define ALLIANCE_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B)
#define HIERARCHY_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)
#define ALLIANCE_BOX_HIGHLIGHT_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define HIERARCHY_BOX_HIGHLIGHT_COLOR HIERARCHY_BACKGROUND_COLOR

#define MESSAGE_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define MESSAGE_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)

enum
{
	FadeAllToWhite = 250,
	FadeSomeToWhite,
	FadeAllToBlack,
	FadeAllToColor,
	FadeSomeToBlack,
	FadeSomeToColor
};

#define SetPlayerIconColor(wp, z) \
	if ((wp) == 0) \
		SetContextForeGroundColor ((COLOR)((z) >= 0 ? \
				BUILD_COLOR (MAKE_RGB15 (0xA, 0x1F, 0x1F), 0x0B) : \
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03))); \
	else \
		SetContextForeGroundColor ((COLOR)((z) >= 0 ? \
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0xA, 0xA), 0x0C) : \
				BUILD_COLOR (MAKE_RGB15 (0x14, 0xA, 0x00), 0x06)))

#define CONTROL_STRING_INDEX 0
#define USING_STRING_INDEX 1
#define RATING_STRING_INDEX 2
#define OK_STRING_INDEX 3
#define LABEL_STRING_COUNT (OK_STRING_INDEX + 1)

#define MENU_REPEAT_DELAY (ONE_SECOND >> 1)

extern ACTIVITY LastActivity;
extern BYTE PlayerControl[NUM_PLAYERS];
extern SIZE cur_player, cur_race;
extern QUEUE disp_q;
extern SIZE battle_counter;
extern FRAME stars_in_space;
extern MEM_HANDLE hResIndex;
extern CONTEXT ScreenContext, StatusContext,
						SpaceContext, OffScreenContext,
						RadarContext, TaskContext;
extern FRAME BackFrame;
extern SIZE screen_width, screen_height;
extern FRAME Screen, RadarFrame;
extern FONT StarConFont, MicroFont, TinyFont;
extern BOOLEAN PagingEnabled;
extern INPUT_REF ArrowInput, ComputerInput, NormalInput, SerialInput,
						JoystickInput[NUM_PLAYERS], KeyboardInput[NUM_PLAYERS],
						CombinedInput[NUM_PLAYERS], PlayerInput[NUM_PLAYERS];
extern FRAME ActivityFrame;
extern SOUND MenuSounds, GameSounds;
extern QUEUE race_q[NUM_PLAYERS];
extern Semaphore GraphicsSem;
extern STRING GameStrings;

typedef enum
{
	PACKAGE_ERROR,
	LOAD_ERROR,
	SAVE_ERROR
} DISK_ERROR;

extern void Introduction (void);
extern BOOLEAN TheftProtection (BOOLEAN WaitForInput);

extern void SetFlashRect (PRECT pRect, FRAME f);

extern void DrawStarConBox (PRECT pRect, SIZE BorderWidth, COLOR
		TopLeftColor, COLOR BottomRightColor, BOOLEAN FillInterior, COLOR
		InteriorColor);
extern INPUT_STATE ConfirmExit (void);
extern DWORD SeedRandomNumbers (void);
extern BOOLEAN StarConDiskError (PSTR pFileName);
extern void ReportDiskError (PSTR pFileName, DISK_ERROR ErrorCondition);
extern void DoInput (PVOID pInputState);
extern INPUT_STATE game_input (INPUT_REF InputRef, INPUT_STATE
		InputState);
extern INPUT_STATE combined_input0 (INPUT_REF InputRef, INPUT_STATE
		InputState);
extern INPUT_STATE combined_input1 (INPUT_REF InputRef, INPUT_STATE
		InputState);
extern BOOLEAN Battle (void);
extern void EncounterBattle (void);
extern void SetPlayerInput (void);
extern BOOLEAN InitContexts (void);
extern void UninitContexts (void);
extern COUNT WrapText (UNICODE *pStr, COUNT len, TEXT *tarray, SIZE
		field_width);

extern void ToggleMusic (void);
extern void PlayMusic (MUSIC_REF MusicRef, BOOLEAN Continuous, BYTE
		Priority);
extern void StopMusic (void);
extern void ResumeMusic (void);
extern void PauseMusic (void);
extern void ToggleSoundEffect (void);
extern void PlaySoundEffect (SOUND S, COUNT Channel, BYTE Priority);

extern BOOLEAN CopyProtection (void);

/* dummy.c */
extern void SetSoundTracking (BOOLEAN toggle);
extern void GetSoundTrackingBuf (PSBYTE pdigi_buf);

extern void StopVid (void);
extern void PlayVid (MEM_HANDLE VidRef, BOOLEAN Continuous);
extern MEM_HANDLE LoadVidFile (PSTR pStr);
extern MEM_HANDLE LoadVidInstance (DWORD res);
extern MEM_HANDLE VidPlaying (void);
extern BOOLEAN DestroyVid (MEM_HANDLE VidRef);

extern DRAWABLE CreatePixmapRegion (FRAME Frame, PPOINT pOrg, SIZE width,
		SIZE height);

extern void SetPrimNextLink (PPRIMITIVE pPrim, COUNT Link);
extern COUNT GetPrimNextLink (PPRIMITIVE pPrim);

//Added by Chris

void AlienTalkSegue (COUNT wait_track);

#endif /* _STARCON_H */

