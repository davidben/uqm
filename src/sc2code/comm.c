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

#include "comm.h"

#include "build.h"
#include "commglue.h"
#include "controls.h"
#include "encount.h"
#include "endian_uqm.h"
#include "gamestr.h"
#include "options.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/inplib.h"
#include "libs/sound/sound.h"
#include "libs/sound/trackplayer.h"
#include "libs/sound/trackint.h"

#include <ctype.h>


void InitOscilloscope (int x, int y, int width, int height, FRAME_DESC *f);
void SetSliderImage (void *f);
void InitSlider (int x, int y, int width, int height,
	int bwidth, int bheight, void *f);
void Oscilloscope (int grab_data);
void Slider (void);


#define MAX_RESPONSES 8
#define BACKGROUND_VOL speechVolumeScale == 0.0f ? MAX_VOLUME : (MAX_VOLUME >> 1)
#define FOREGROUND_VOL MAX_VOLUME

#define SLIDER_Y 107
#define SLIDER_HEIGHT 15


LOCDATA CommData;
int cur_comm;
UNICODE shared_phrase_buf[256];
volatile BOOLEAN summary = FALSE;

typedef struct response_entry
{
	RESPONSE_REF response_ref;
	TEXT response_text;
	RESPONSE_FUNC response_func;
} RESPONSE_ENTRY;
typedef RESPONSE_ENTRY *PRESPONSE_ENTRY;

typedef struct encounter_state
{
	BOOLEAN (*InputFunc) (struct encounter_state *pES);
	COUNT MenuRepeatDelay;

	COUNT Initialized;
	BYTE num_responses, cur_response, top_response;
	RESPONSE_ENTRY response_list[MAX_RESPONSES];

	Task AnimTask;

	COUNT phrase_buf_index;
	UNICODE phrase_buf[512];
} ENCOUNTER_STATE;
typedef ENCOUNTER_STATE *PENCOUNTER_STATE;

enum
{
	UP_DIR,
	DOWN_DIR,
	NO_DIR
};
enum
{
	PICTURE_ANIM,
	COLOR_ANIM
};
typedef struct
{
	COUNT Alarm;
	BYTE Direction, FramesLeft;
	BYTE AnimType;
	union
	{
		FRAME CurFrame;
		COLORMAP CurCMap;
	} AnimObj;
} SEQUENCE, *PSEQUENCE;

static PENCOUNTER_STATE pCurInputState;
enum
{
	DONE_SUBTITLE,
	NEXT_SUBTITLE,
	READ_SUBTITLE,
	SPACE_SUBTITLE,
	WAIT_SUBTITLE,
};
static int subtitle_state = DONE_SUBTITLE;
static Mutex subtitle_mutex;

static volatile UNICODE *last_subtitle;
static TFB_Image *subtitle_cache;

/* _count_lines - mostly stolen from add_text, just sees how many lines
                  a given input string would take to display given the
                  line wrapping information */
static int _count_lines (PTEXT pText) 
{
	COUNT maxchars = (COUNT)~0;
	UNICODE ch, *pStr;
	SIZE text_width;
	int num = 0;

	text_width = CommData.AlienTextWidth;
	SetContextFont (CommData.AlienFont);

	pStr = pText->pStr;

	do
	{
		++num;
		pText->pStr = pStr;
		pText->CharCount = 1;

		{
			BOOLEAN eot;
			RECT r, old_r;
			COUNT OldCount;

			GetContextClipRect (&r);
			eot = FALSE;
			do
			{
				old_r = r;
				OldCount = pText->CharCount;
				while (!(eot = (BOOLEAN)(
						(ch = *++pStr) == '\0'
						|| ch == '\n'
						|| ch == '\r'
						|| (COUNT)(pStr - pText->pStr) >= maxchars
						)) && ch != ' ')
					;
				pText->CharCount = pStr - pText->pStr;
				TextRect (pText, &r, NULL_PTR);
			} while (!eot && r.extent.width < text_width);

			if (r.extent.width >= text_width)
			{
				pText->CharCount = OldCount;
				r = old_r;
			}
		}
		pStr = pText->pStr + pText->CharCount;
	} while ((ch = *pStr++) != '\0' && ch != '\n' && ch != '\r' && maxchars);
	pText->pStr = pStr;

	return (num);
}

static COORD
add_text (int status, PTEXT pTextIn)
{
	COUNT maxchars, numchars;
	TEXT locText;
	PTEXT pText;
	SIZE leading;
	UNICODE ch, *pStr;
	SIZE text_width;
	int num_lines = 0;
	static COORD last_baseline;
	
	BatchGraphics ();

	maxchars = (COUNT)~0;
	if (status == 1)
	{
		RECT r;
		GetContextClipRect (&r);

		if (last_subtitle == pTextIn->pStr)
		{
			// draws cached subtitle
			TFB_DrawScreen_Image (subtitle_cache, r.corner.x, r.corner.y, 0, 0, TFB_SCREEN_MAIN);
			UnbatchGraphics ();
			return last_baseline;
		}
		else
		{
			// saves background to extra screen
			TFB_DrawScreen_Copy (&r, TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA);
			// fills screen with transparent color
			TFB_DrawScreen_Rect (&r, 0, 0, 128, TFB_SCREEN_MAIN);

			last_subtitle = pTextIn->pStr;
		}

		text_width = CommData.AlienTextWidth;
		SetContextFont (CommData.AlienFont);
		GetContextFontLeading (&leading);

		pText = pTextIn;
	}
	else if (GetContextFontLeading (&leading), status <= -4)
	{
		text_width = (SIZE) (SIS_SCREEN_WIDTH - 8 - (TEXT_X_OFFS << 2));

		pText = pTextIn;
	}
	else
	{
		text_width = (SIZE) (SIS_SCREEN_WIDTH - 8 - (TEXT_X_OFFS << 2));

		switch (status)
		{
			case -3:
				SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
				break;
			case -2:
				SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
				break;
			case -1:
				SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1A, 0x1A, 0x1A), 0x12));
				break;
		}

		maxchars = pTextIn->CharCount;
		locText = *pTextIn;
		locText.baseline.x -= 8;
		locText.CharCount = 1;
		ch = '*';
		locText.pStr = &ch;
		font_DrawText (&locText);

		locText = *pTextIn;
		pText = &locText;
		pText->baseline.y -= leading;
	}

	numchars = 0;
	pStr = pText->pStr;

	if (status > 0 && (CommData.AlienTextTemplate.valign & (VALIGN_MIDDLE|VALIGN_BOTTOM))) {
		num_lines = _count_lines(pText);
		if (CommData.AlienTextTemplate.valign == VALIGN_BOTTOM)
			pText->baseline.y -= (leading * num_lines);
		else if (CommData.AlienTextTemplate.valign == VALIGN_MIDDLE)
			pText->baseline.y -= ((leading * num_lines) / 2);
		if (pText->baseline.y < 0) pText->baseline.y = 0;
	}

	do
	{
		pText->pStr = pStr;
		pText->CharCount = 1;
		pText->baseline.y += leading;

		{
			BOOLEAN eot;
			RECT r, old_r;
			COUNT OldCount;

			GetContextClipRect (&r);
			eot = FALSE;
			do
			{
				old_r = r;
				OldCount = pText->CharCount;
				while (!(eot = (BOOLEAN)(
						(ch = *++pStr) == '\0'
						|| ch == '\n'
						|| ch == '\r'
						|| (COUNT)(pStr - pText->pStr) >= maxchars
						)) && ch != ' ')
					;
				pText->CharCount = pStr - pText->pStr;
				TextRect (pText, &r, NULL_PTR);
			} while (!eot && r.extent.width < text_width);

			if (r.extent.width >= text_width)
			{
				pText->CharCount = OldCount;
				r = old_r;
			}

			if (maxchars -= pText->CharCount)
				--maxchars;
			numchars += pText->CharCount;
			
			if (status <= 0)
			{
				if (pText->baseline.y < SIS_SCREEN_HEIGHT)
					font_DrawText (pText);

				pStr = pText->pStr + pText->CharCount;
				if(status < -4 && pText->baseline.y >= -status - 10)
				{
					++pStr;
					break;
				}
			}
			else
			{
				SetContextForeGroundColor (CommData.AlienTextBColor);

				--pText->baseline.x;
				font_DrawText (pText);

				++pText->baseline.x;
				--pText->baseline.y;
				font_DrawText (pText);

				++pText->baseline.x;
				++pText->baseline.y;
				font_DrawText (pText);

				--pText->baseline.x;
				++pText->baseline.y;
				font_DrawText (pText);

				SetContextForeGroundColor (CommData.AlienTextFColor);

				--pText->baseline.y;
				font_DrawText (pText);

				pStr = pText->pStr + pText->CharCount;
			}
		}
	} while ((ch = *pStr++) != '\0' && ch != '\n' && ch != '\r' && maxchars);
	pText->pStr = pStr;

	if (status == 1)
	{
		RECT r;
		GetContextClipRect (&r);
		
		// copies subtitle to cache
		TFB_DrawScreen_CopyToImage (subtitle_cache, &r, TFB_SCREEN_MAIN);
		// restores background
		TFB_DrawScreen_Copy (&r, TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN);
		// draws cached subtitle
		TFB_DrawScreen_Image (subtitle_cache, r.corner.x, r.corner.y, 0, 0, TFB_SCREEN_MAIN);
		
		last_baseline = pText->baseline.y;
	}

	UnbatchGraphics ();
	return (pText->baseline.y);
}

static void
DrawSISComWindow (void)
{
	CONTEXT OldContext;

	if (LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
	{
		RECT r;

		OldContext = SetContext (SpaceContext);

		r.corner.x = 0;
		r.corner.y = SLIDER_Y + SLIDER_HEIGHT;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;
		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
		DrawFilledRectangle (&r);

		SetContext (OldContext);
	}
#ifdef NEVER
	else
	{
#define NUM_CREDITS 5
		BYTE j;
		TEXT t;
		STRING OldStrings;
		extern STRING CreditStrings;

		t.baseline.x = SCREEN_WIDTH >> 1;
		t.baseline.y = RADAR_Y - 5;
		t.align = ALIGN_CENTER;

		OldContext = SetContext (ScreenContext);

		OldStrings = SetRelStringTableIndex (CreditStrings, -NUM_CREDITS);
		SetContextFont (MicroFont);
		BatchGraphics ();
		for (j = 0; j < NUM_CREDITS; ++j)
		{
			SetContextForeGroundColor (BLACK_COLOR);
			t.pStr = (UNICODE *)GetStringAddress (OldStrings);
			t.CharCount = GetStringLength (OldStrings);
			OldStrings = SetRelStringTableIndex (OldStrings, 1);
			font_DrawText (&t);
			SetContextForeGroundColor (WHITE_COLOR);
			t.pStr = (PBYTE)GetStringAddress (CreditStrings);
			t.CharCount = GetStringLength (CreditStrings);
			CreditStrings = SetRelStringTableIndex (CreditStrings, 1);
			font_DrawText (&t);

			if (j)
				t.baseline.y += 12;
			else
			{
				t.baseline.y += 16;
				SetContextFont (StarConFont);
			}
		}
		UnbatchGraphics ();

		SetContext (OldContext);
	}
#endif /* NEVER */
}

static void
DrawAlienFrame (FRAME aframe, PSEQUENCE pSeq)
{
	COUNT i;
	STAMP s;
	ANIMATION_DESCPTR ADPtr;

	s.origin.x = -SAFE_X;
	s.origin.y = 0;
	if ((s.frame = CommData.AlienFrame) == 0)
		s.frame = aframe;
	
	BatchGraphics ();
	DrawStamp (&s);
	ADPtr = &CommData.AlienAmbientArray[i = CommData.NumAnimations];
	while (i--)
	{
		--ADPtr;

		if (!(ADPtr->AnimFlags & ANIM_MASK))
		{
			s.frame = SetAbsFrameIndex (
					s.frame,
					ADPtr->StartIndex
					);
			DrawStamp (&s);
			ADPtr->AnimFlags |= ANIM_DISABLED;
		}
		else if (pSeq)
		{
			if (pSeq->AnimType == PICTURE_ANIM)
			{
				s.frame = pSeq->AnimObj.CurFrame;
				DrawStamp (&s);
			}
			--pSeq;
		}
	}
	if (aframe && CommData.AlienFrame && aframe != CommData.AlienFrame)
	{
		s.frame = aframe;
		DrawStamp (&s);
	}
	UnbatchGraphics ();
}

typedef struct xform_control
{
	COLORMAPPTR CMapPtr;
	SIZE Ticks, TTotal, TOrig;
	DWORD OldCMap[NUMBER_OF_PLUT_UINT32s];
} XFORM_CONTROL;

#define MAX_XFORMS 32
static struct {
	XFORM_CONTROL TaskControl[MAX_XFORMS];
	volatile int XFormCurrent, XFormInsertPoint;
	volatile BOOLEAN XFormsPending;
	Mutex XFormLock;
} XFormControl;

void
init_xform_control (void)
{
	XFormControl.XFormCurrent = XFormControl.XFormInsertPoint = 0;
	XFormControl.XFormsPending = FALSE;
	XFormControl.XFormLock = CreateMutex ("Transform Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
}

void
uninit_xform_control (void)
{
	DestroyMutex (XFormControl.XFormLock);
}

void
xform_complete (void)
{
	LockMutex (XFormControl.XFormLock);
	if (XFormControl.XFormsPending)
	{
		SetColorMap (XFormControl.TaskControl[XFormControl.XFormCurrent].CMapPtr);
		if (++XFormControl.XFormCurrent >= MAX_XFORMS)
		{
			XFormControl.XFormCurrent = 0;
		}
		if (XFormControl.XFormCurrent == XFormControl.XFormInsertPoint)
		{
			XFormControl.XFormsPending = FALSE;
		}
	}
	UnlockMutex (XFormControl.XFormLock);
}

void
init_communication (void)
{
	TFB_Canvas canvas;

	subtitle_mutex = CreateMutex ("Subtitle Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
	init_xform_control ();

	canvas = TFB_DrawCanvas_New_TrueColor (SIS_SCREEN_WIDTH,
		SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT + 2, FALSE);
	subtitle_cache = TFB_DrawImage_New (canvas);
	TFB_DrawCanvas_SetTransparentColor (subtitle_cache->NormalImg, 0, 0, 128, TRUE);
}

void
uninit_communication (void)
{
	DestroyMutex (subtitle_mutex);
	uninit_xform_control ();
	TFB_DrawImage_Delete (subtitle_cache);
	subtitle_cache = NULL;
}

static volatile BOOLEAN ColorChange;
static volatile BOOLEAN SummaryChange;
static volatile BOOLEAN ClearSubtitle;

/* Only one thread should ever be allowed to be calling this at any time */
void
xform_PLUT_step (SIZE TDelta)
{
	XFORM_CONTROL *control;
	COLORMAPPTR ColorMapPtr;
	DWORD *pCurCMap, *pOldCMap;
	int i;

	if (!XFormControl.XFormsPending)
		return;

	control = &XFormControl.TaskControl[XFormControl.XFormCurrent];

	ColorMapPtr = control->CMapPtr;

	if (TDelta > control->TTotal)
		TDelta = control->TTotal;

	pCurCMap = (DWORD *)((BYTE *)_varPLUTs + (*ColorMapPtr * PLUT_BYTE_SIZE));
	pOldCMap = control->OldCMap;

	ColorMapPtr += 2;
	if (_varPLUTs)
	{
		for (i = 0; i < NUMBER_OF_PLUT_UINT32s; i++)
		{
			SIZE c0, c1;
			DWORD v0, v1, val;
			float f = (control->TOrig - control->TTotal) / (float)control->TOrig;
			COLORMAPPTR oldmap = (COLORMAPPTR) pOldCMap;

			v0 = UQM_SwapBE32 (*(DWORD *)oldmap);
			v1 = UQM_SwapBE32 (*(DWORD *)ColorMapPtr);

			ColorMapPtr += sizeof (DWORD);
			
			c0 = (SIZE)((v0 >> (10 + 16)) & 0x1F);
			c1 = (SIZE)((v1 >> (10 + 16)) & 0x1F);
			c0 += (SIZE)((c1 - c0) * f);
			val = (c0 & 0x1F) | (1 << 5);

			c0 = (SIZE)((v0 >> (5 + 16)) & 0x1F);
			c1 = (SIZE)((v1 >> (5 + 16)) & 0x1F);
			c0 += (SIZE)((c1 - c0) * f);
			val = (val << 5) | (c0 & 0x1F);

			c0 = (SIZE)((v0 >> (0 + 16)) & 0x1F);
			c1 = (SIZE)((v1 >> (0 + 16)) & 0x1F);
			c0 += (SIZE)((c1 - c0) * f);
			val = (val << 5) | (c0 & 0x1F);

			c0 = (SIZE)((v0 >> 10) & 0x1F);
			c1 = (SIZE)((v1 >> 10) & 0x1F);
			c0 += (SIZE)((c1 - c0) * f);
			val = (val << (5 + 1)) | (c0 & 0x1F) | (1 << 5);

			c0 = (SIZE)((v0 >> 5) & 0x1F);
			c1 = (SIZE)((v1 >> 5) & 0x1F);
			c0 += (SIZE)((c1 - c0) * f);
			val = (val << 5) | (c0 & 0x1F);
			
			c0 = (SIZE)((v0 >> 0) & 0x1F);
			c1 = (SIZE)((v1 >> 0) & 0x1F);
			c0 += (SIZE)((c1 - c0) * f);
			val = (val << 5) | (c0 & 0x1F);

			pOldCMap++;
			*pCurCMap++ = UQM_SwapBE32 (val);
		}
		ColorChange = TRUE;
	}
	control->TTotal -= TDelta;
	if (!control->TTotal)
	{
		xform_complete ();
	}
}

/* This should be thread-safe without locking the XFormControl because
 * only one thread should be Flushing at any given time, constant
 * writes are atomic anyway, and Flush is the only routine that writes
 * XFormFlush. */

void
FlushPLUTXForms (void)
{
	while (XFormControl.XFormsPending)
	{
		xform_complete ();
	}
}

DWORD
XFormPLUT (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	if (ColorMapPtr)
	{
		DWORD *pOldCMap, *pCurCMap;
		XFORM_CONTROL *control;
		int i;

		// FlushPLUTXForms ();

		LockMutex (XFormControl.XFormLock);
		while (XFormControl.XFormsPending
				&& (XFormControl.XFormInsertPoint == XFormControl.XFormCurrent))
		{
			UnlockMutex (XFormControl.XFormLock);
			FlushPLUTXForms ();
			LockMutex (XFormControl.XFormLock);
		}
		
		control = &XFormControl.TaskControl[XFormControl.XFormInsertPoint];

		pCurCMap = (DWORD *)((BYTE *)_varPLUTs + (*ColorMapPtr * PLUT_BYTE_SIZE));
		pOldCMap = control->OldCMap;
		for (i = 0; i < NUMBER_OF_PLUT_UINT32s; ++i)
			*pOldCMap++ = *pCurCMap++;

		control->CMapPtr = ColorMapPtr;
		if ((control->Ticks = TimeInterval) <= 0)
			control->Ticks = 1; /* prevent divide by zero and negative fade */
		control->TTotal = control->TOrig = TimeInterval;

		if (++XFormControl.XFormInsertPoint >= MAX_XFORMS)
			XFormControl.XFormInsertPoint = 0;

		XFormControl.XFormsPending = TRUE;

		UnlockMutex (XFormControl.XFormLock);		

	}

	return (0);
}

static void
RefreshResponses (PENCOUNTER_STATE pES)
{
	COORD y;
	BYTE response;
	SIZE leading;
	STAMP s;

	SetContext (SpaceContext);
	GetContextFontLeading (&leading);
	BatchGraphics ();

	DrawSISComWindow ();
	y = SLIDER_Y + SLIDER_HEIGHT + 1;
	for (response = pES->top_response; response < pES->num_responses; ++response)
	{
		pES->response_list[response].response_text.baseline.x = TEXT_X_OFFS + 8;
		pES->response_list[response].response_text.baseline.y = y + leading;
		pES->response_list[response].response_text.align = ALIGN_LEFT;
		if (response == pES->cur_response)
			y = add_text (-1, &pES->response_list[response].response_text);
		else
			y = add_text (-2, &pES->response_list[response].response_text);
	}

	if (pES->top_response)
	{
		s.origin.y = SLIDER_Y + SLIDER_HEIGHT + 1;
		s.frame = SetAbsFrameIndex (ActivityFrame, 6);
	}
	else if (y > SIS_SCREEN_HEIGHT)
	{
		s.origin.y = SIS_SCREEN_HEIGHT - 2;
		s.frame = SetAbsFrameIndex (ActivityFrame, 7);
	}
	else
		s.frame = 0;
	if (s.frame)
	{
		RECT r;

		GetFrameRect (s.frame, &r);
		s.origin.x = SIS_SCREEN_WIDTH - r.extent.width - 1;
		DrawStamp (&s);
	}

	UnbatchGraphics ();
}

static void
FeedbackPlayerPhrase (UNICODE *pStr)
{
	last_subtitle = NULL;

	SetContext (SpaceContext);
	
	BatchGraphics ();
	DrawSISComWindow ();
	if (pStr[0])
	{
		TEXT ct;

		ct.baseline.x = SIS_SCREEN_WIDTH >> 1;
		ct.baseline.y = SLIDER_Y + SLIDER_HEIGHT + 13;
		ct.align = ALIGN_CENTER;
		ct.CharCount = (COUNT)~0;
		ct.pStr = GAME_STRING (FEEDBACK_STRING_BASE);

		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0xA, 0xC, 0x1F), 0x48));
		font_DrawText (&ct);
		ct.baseline.y += 16;
		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x12, 0x14, 0x4F), 0x44));
		ct.pStr = pStr;
		add_text (-4, &ct);
	}
	UnbatchGraphics ();
}

static void
SetUpSequence (PSEQUENCE pSeq)
{
	COUNT i;
	ANIMATION_DESCPTR ADPtr;

	pSeq = &pSeq[i = CommData.NumAnimations];
	ADPtr = &CommData.AlienAmbientArray[i];
	while (i--)
	{
		--ADPtr;
		--pSeq;

		if (ADPtr->AnimFlags & COLORXFORM_ANIM)
			pSeq->AnimType = COLOR_ANIM;
		else
			pSeq->AnimType = PICTURE_ANIM;
		pSeq->Direction = UP_DIR;
		pSeq->FramesLeft = ADPtr->NumFrames;
		if (pSeq->AnimType == COLOR_ANIM)
			pSeq->AnimObj.CurCMap = SetAbsColorMapIndex (
					CommData.AlienColorMap,
					ADPtr->StartIndex
					);
		else
			pSeq->AnimObj.CurFrame = SetAbsFrameIndex (
					CommData.AlienFrame,
					ADPtr->StartIndex
					);

		if (ADPtr->AnimFlags & RANDOM_ANIM)
		{
			if (pSeq->AnimType == COLOR_ANIM)
				pSeq->AnimObj.CurCMap =
						SetRelColorMapIndex (pSeq->AnimObj.CurCMap,
						(COUNT)((COUNT)TFB_Random () % pSeq->FramesLeft));
			else
				pSeq->AnimObj.CurFrame =
						SetRelFrameIndex (pSeq->AnimObj.CurFrame,
						(COUNT)((COUNT)TFB_Random () % pSeq->FramesLeft));
		}
		else if (ADPtr->AnimFlags & YOYO_ANIM)
		{
			--pSeq->FramesLeft;
			if (pSeq->AnimType == COLOR_ANIM)
				pSeq->AnimObj.CurCMap =
						SetRelColorMapIndex (pSeq->AnimObj.CurCMap, 1);
			else
				pSeq->AnimObj.CurFrame =
						IncFrameIndex (pSeq->AnimObj.CurFrame);
		}

		pSeq->Alarm =
				ADPtr->BaseRestartRate
				+ ((COUNT)TFB_Random ()
				% (ADPtr->RandomRestartRate + 1))
				+ 1;
	}
}

static void
UpdateSpeechGraphics (BOOLEAN Initialize)
{
	CONTEXT OldContext;

	if (Initialize)
	{
		RECT r, sr;
		FRAME f;

		InitOscilloscope (0, 0, RADAR_WIDTH, RADAR_HEIGHT,
				SetAbsFrameIndex (ActivityFrame, 9));
		f = SetAbsFrameIndex (ActivityFrame, 2);
		GetFrameRect (f, &r);
		SetSliderImage (f);
		f = SetAbsFrameIndex (ActivityFrame, 5);
		GetFrameRect (f, &sr);
		InitSlider (0, SLIDER_Y, SIS_SCREEN_WIDTH, sr.extent.height,
				r.extent.width, r.extent.height, f);
	}

	OldContext = SetContext (RadarContext);
	Oscilloscope (!Initialize);
	SetContext (SpaceContext);
	Slider ();
	SetContext (OldContext);
}

int ambient_anim_task(void* data)
{
	SIZE TalkAlarm;
	FRAME TalkFrame;
	FRAME ResetTalkFrame = NULL;
	FRAME TransitionFrame = NULL;
	FRAME AnimFrame[MAX_ANIMATIONS + 1];
	COUNT i;
	DWORD LastTime;
	FRAME CommFrame;
	SEQUENCE Sequencer[MAX_ANIMATIONS];
	ANIMATION_DESC TalkBuffer = CommData.AlienTalkDesc;
	PSEQUENCE pSeq;
	ANIMATION_DESCPTR ADPtr;
	DWORD ActiveMask;
	DWORD LastOscillTime;
	Task task = (Task) data;
	BOOLEAN TransitionDone = FALSE;
	BOOLEAN TalkFrameChanged = FALSE;
	BOOLEAN FrameChanged[MAX_ANIMATIONS];

	while ((CommFrame = CommData.AlienFrame) == 0 && !Task_ReadState (task, TASK_EXIT))
		TaskSwitch ();

	LockMutex (GraphicsLock);
	memset ((PSTR)&DisplayArray[0], 0, sizeof (DisplayArray));
	SetUpSequence (Sequencer);
	UnlockMutex (GraphicsLock);

	ActiveMask = 0;
	TalkAlarm = 0;
	TalkFrame = 0;
	LastTime = GetTimeCounter ();
	LastOscillTime = LastTime;
	memset (&FrameChanged[0], 0 , MAX_ANIMATIONS);
	memset (&AnimFrame[0], 0, sizeof (FRAME) * MAX_ANIMATIONS);
	for (i = 0; i <= CommData.NumAnimations; i++)
		if (CommData.AlienAmbientArray[i].AnimFlags & YOYO_ANIM)
			AnimFrame[i] =  SetAbsFrameIndex (
					CommFrame,
					CommData.AlienAmbientArray[i].StartIndex
					);
		else
			AnimFrame[i] =  SetAbsFrameIndex (
					CommFrame,
					(COUNT)(CommData.AlienAmbientArray[i].StartIndex
					+ CommData.AlienAmbientArray[i].NumFrames - 1)
					);

	while (!Task_ReadState (task, TASK_EXIT))
	{
		BOOLEAN Change, CanTalk;
		DWORD CurTime, ElapsedTicks;

		SleepThreadUntil (LastTime + ONE_SECOND / 120);

		LockMutex (GraphicsLock);
		BatchGraphics ();
		CurTime = GetTimeCounter ();
		ElapsedTicks = CurTime - LastTime;
		LastTime = CurTime;

		Change = FALSE;
		i = CommData.NumAnimations;
		if (CommData.AlienFrame)
			CanTalk = TRUE;
		else
		{
			i = 0;
			CanTalk = FALSE;
		}

		pSeq = &Sequencer[i];
		ADPtr = &CommData.AlienAmbientArray[i];
		while (i-- && !Task_ReadState (task, TASK_EXIT))
		{
			--ADPtr;
			--pSeq;
			if (ADPtr->AnimFlags & ANIM_DISABLED)
				continue;
			if (pSeq->Direction == NO_DIR)
			{
				if (!(ADPtr->AnimFlags
						& CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING))
					pSeq->Direction = UP_DIR;
			}
			else if ((DWORD)pSeq->Alarm > ElapsedTicks)
				pSeq->Alarm -= (COUNT)ElapsedTicks;
			else
			{
				if (!(ActiveMask & ADPtr->BlockMask)
						&& (--pSeq->FramesLeft
						|| ((ADPtr->AnimFlags & YOYO_ANIM)
						&& pSeq->Direction == UP_DIR)))
				{
					ActiveMask |= 1L << i;
					pSeq->Alarm =
							ADPtr->BaseFrameRate
							+ ((COUNT)TFB_Random ()
							% (ADPtr->RandomFrameRate + 1))
							+ 1;
				}
				else
				{
					ActiveMask &= ~(1L << i);
					pSeq->Alarm =
							ADPtr->BaseRestartRate
							+ ((COUNT)TFB_Random ()
							% (ADPtr->RandomRestartRate + 1))
							+ 1;
					if (ActiveMask & ADPtr->BlockMask)
						continue;
				}

				if (pSeq->AnimType == COLOR_ANIM)
				{
					XFormPLUT (
							GetColorMapAddress (pSeq->AnimObj.CurCMap),
							(COUNT) (pSeq->Alarm - 1)
							);
				}
				else
				{
					Change = TRUE;
					AnimFrame[i] = pSeq->AnimObj.CurFrame;
					FrameChanged[i] = 1;
				}

				if (pSeq->FramesLeft == 0)
				{
					pSeq->FramesLeft = (BYTE)(ADPtr->NumFrames - 1);

					if (pSeq->Direction == DOWN_DIR)
						pSeq->Direction = UP_DIR;
					else if (ADPtr->AnimFlags & YOYO_ANIM)
						pSeq->Direction = DOWN_DIR;
					else
					{
						++pSeq->FramesLeft;
						if (pSeq->AnimType == COLOR_ANIM)
							pSeq->AnimObj.CurCMap = SetRelColorMapIndex (
									pSeq->AnimObj.CurCMap,
									(SWORD) (-pSeq->FramesLeft)
									);
						else
						{
							pSeq->AnimObj.CurFrame = SetRelFrameIndex (
									pSeq->AnimObj.CurFrame,
									(SWORD) (-pSeq->FramesLeft)
									);
						}
					}
				}

				if (ADPtr->AnimFlags & RANDOM_ANIM)
				{
					if (pSeq->AnimType == COLOR_ANIM)
						pSeq->AnimObj.CurCMap =
								SetAbsColorMapIndex (pSeq->AnimObj.CurCMap,
								(COUNT) (ADPtr->StartIndex
								+ ((COUNT)TFB_Random ()
								% ADPtr->NumFrames)));
					else
						pSeq->AnimObj.CurFrame =
								SetAbsFrameIndex (pSeq->AnimObj.CurFrame,
								(COUNT) (ADPtr->StartIndex
								+ ((COUNT)TFB_Random ()
								% ADPtr->NumFrames)));
				}
				else if (pSeq->AnimType == COLOR_ANIM)
				{
					if (pSeq->Direction == UP_DIR)
						pSeq->AnimObj.CurCMap = SetRelColorMapIndex (
								pSeq->AnimObj.CurCMap, 1
								);
					else
						pSeq->AnimObj.CurCMap = SetRelColorMapIndex (
								pSeq->AnimObj.CurCMap, -1
								);
				}
				else
				{
					if (pSeq->Direction == UP_DIR)
						pSeq->AnimObj.CurFrame =
								IncFrameIndex (pSeq->AnimObj.CurFrame);
					else
						pSeq->AnimObj.CurFrame =
								DecFrameIndex (pSeq->AnimObj.CurFrame);
				}
			}

			if (pSeq->AnimType == PICTURE_ANIM
					&& (ADPtr->AnimFlags
					& CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING)
					&& pSeq->Direction != NO_DIR)
			{
				COUNT index;

				CanTalk = FALSE;
				if (!(pSeq->Direction != UP_DIR
						|| (index = GetFrameIndex (pSeq->AnimObj.CurFrame)) >
						ADPtr->StartIndex + 1
						|| (index == ADPtr->StartIndex + 1
						&& (ADPtr->AnimFlags & CIRCULAR_ANIM))))
					pSeq->Direction = NO_DIR;
			}
		}

		ADPtr = &CommData.AlienTalkDesc;
		if (CanTalk
				&& ADPtr->NumFrames
				&& (ADPtr->AnimFlags & WAIT_TALKING)
				&& !(CommData.AlienTransitionDesc.AnimFlags & PAUSE_TALKING))
		{
			BOOLEAN done = 0;
			for (i = 0; i < CommData.NumAnimations; i++)
				if (ActiveMask & (1L << i) 
					&& CommData.AlienAmbientArray[i].AnimFlags & WAIT_TALKING)
					done = 1;
			if (! done)
			{

				if (ADPtr->StartIndex != TalkBuffer.StartIndex)
				{
					Change = TRUE;
					ResetTalkFrame = SetAbsFrameIndex (CommFrame,
							TalkBuffer.StartIndex);
					TalkBuffer = CommData.AlienTalkDesc;
				}

				if ((long)TalkAlarm > (long)ElapsedTicks)
					TalkAlarm -= (SIZE)ElapsedTicks;
				else
				{
					BYTE AFlags;
					SIZE FrameRate;

					if (TalkAlarm > 0)
						TalkAlarm = 0;
					else
						TalkAlarm = -1;

					AFlags = ADPtr->AnimFlags;
					if (!(AFlags & (TALK_INTRO | TALK_DONE)))
					{
						FrameRate =
								ADPtr->BaseFrameRate
								+ ((COUNT)TFB_Random ()
								% (ADPtr->RandomFrameRate + 1));
						if (TalkAlarm < 0
								|| GetFrameIndex (TalkFrame) ==
								ADPtr->StartIndex)
						{
							TalkFrame = SetAbsFrameIndex (CommFrame,
									(COUNT) (ADPtr->StartIndex + 1
									+ ((COUNT)TFB_Random ()
									% (ADPtr->NumFrames - 1))));
							FrameRate +=
									ADPtr->BaseRestartRate
									+ ((COUNT)TFB_Random ()
									% (ADPtr->RandomRestartRate + 1));
						}
						else
						{
							TalkFrame = SetAbsFrameIndex (CommFrame,
									ADPtr->StartIndex);
							if (ADPtr->AnimFlags & PAUSE_TALKING)
							{
								if (!(CommData.AlienTransitionDesc.AnimFlags
										& TALK_DONE))
								{
									CommData.AlienTransitionDesc.AnimFlags |=
											PAUSE_TALKING;
									ADPtr->AnimFlags &=
											~PAUSE_TALKING;
								}
								else if (CommData.AlienTransitionDesc.NumFrames)
									ADPtr->AnimFlags |=
											TALK_DONE;
								else
									ADPtr->AnimFlags &=
											~(WAIT_TALKING | PAUSE_TALKING);

								FrameRate = 0;
							}
						}
					}
					else
					{
						ADPtr = &CommData.AlienTransitionDesc;
						if (AFlags & TALK_INTRO)
						{
							FrameRate =
									ADPtr->BaseFrameRate
									+ ((COUNT)TFB_Random ()
									% (ADPtr->RandomFrameRate + 1));
							if (TalkAlarm < 0 || TransitionDone)
							{
								TalkFrame = SetAbsFrameIndex (CommFrame,
										ADPtr->StartIndex);
								TransitionDone = 0;
							}
							else
								TalkFrame = IncFrameIndex (TalkFrame);

							if ((BYTE)(GetFrameIndex (TalkFrame)
									- ADPtr->StartIndex + 1) ==
									ADPtr->NumFrames)
							{
								CommData.AlienTalkDesc.AnimFlags &= ~TALK_INTRO;
								TransitionDone = 1;
							}
							TransitionFrame = TalkFrame;
						}
						else /* if (AFlags & TALK_DONE) */
						{
							FrameRate =
									ADPtr->BaseFrameRate
									+ ((COUNT)TFB_Random ()
									% (ADPtr->RandomFrameRate + 1));
							if (TalkAlarm < 0)
								TalkFrame = SetAbsFrameIndex (CommFrame,
										(COUNT) (ADPtr->StartIndex
										+ ADPtr->NumFrames - 1));
							else
								TalkFrame = DecFrameIndex (TalkFrame);

							if (GetFrameIndex (TalkFrame) ==
									ADPtr->StartIndex)
							{
								CommData.AlienTalkDesc.AnimFlags &=
										~(PAUSE_TALKING | TALK_DONE);
								if (ADPtr->AnimFlags & TALK_INTRO)
									CommData.AlienTalkDesc.AnimFlags &= ~WAIT_TALKING;
								else
								{
									ADPtr->AnimFlags |=
											PAUSE_TALKING;
									ADPtr->AnimFlags &= ~TALK_DONE;
								}
								FrameRate = 0;
							}
							TransitionFrame = NULL;
						}
					}
					TalkFrameChanged = TRUE;

					Change = TRUE;

					TalkAlarm = FrameRate;
				}
			}
			else if (done && (ADPtr->AnimFlags & PAUSE_TALKING))
			{
				ADPtr->AnimFlags &= ~(WAIT_TALKING | PAUSE_TALKING);				
			}
		}

		if (!summary)
		{
			CONTEXT OldContext;
			BOOLEAN CheckSub = 0;
			BOOLEAN ClearSub;
			int sub_state;

			LockMutex (subtitle_mutex);
			ClearSub = ClearSubtitle;
			sub_state = subtitle_state;
			ClearSubtitle = FALSE;
			UnlockMutex (subtitle_mutex);


			OldContext = SetContext (TaskContext);

			if (ColorChange || SummaryChange)
			{
				FRAME F;
				F = CommData.AlienFrame;
				CommData.AlienFrame = CommFrame;
				DrawAlienFrame (TalkFrame, &Sequencer[CommData.NumAnimations - 1]);
				CommData.AlienFrame = F;
				CheckSub = 1;
				ClearSub = SummaryChange ? TRUE : FALSE;
				ColorChange = SummaryChange = FALSE;
			}
			if (Change || ClearSub)
			{
				STAMP s;
				s.origin.x = -SAFE_X;
				s.origin.y = 0;
				if (ClearSub)
				{
					s.frame = CommFrame;
					DrawStamp (&s);
				}
				i = CommData.NumAnimations;
				while (i--)
				{
					if ((ClearSub || FrameChanged[i]))
					{
						s.frame = AnimFrame[i];
						DrawStamp (&s);
						FrameChanged[i] = 0;
					}
				}
				if (ClearSub && TransitionFrame)
				{
					s.frame = TransitionFrame;
					DrawStamp (&s);
				}
				if (ResetTalkFrame)
				{
					s.frame = ResetTalkFrame;
					DrawStamp (&s);
					ResetTalkFrame = NULL;
				}
				if (TalkFrame && TalkFrameChanged)
				{
					s.frame = TalkFrame;
					DrawStamp (&s);
					TalkFrameChanged = FALSE;
				}
				Change = FALSE;
				CheckSub = 1;
			}

			if (optSubtitles && CheckSub && sub_state >= SPACE_SUBTITLE)
			{
				TEXT t;

				t = CommData.AlienTextTemplate;
				add_text (1, &t);
			}

			SetContext (OldContext);
		}
		if (LastOscillTime + (ONE_SECOND / 32) < CurTime)
		{
			LastOscillTime = CurTime;
			UpdateSpeechGraphics (FALSE);
		}
		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);
		xform_PLUT_step(ElapsedTicks);
	}
	FinishTask (task);
	return(0);
}

static BOOLEAN
SpewPhrases (COUNT wait_track)
{
	BOOLEAN ContinuityBreak;
	DWORD TimeIn;
	COUNT which_track;
	FRAME F;
	BOOLEAN passed = TRUE;

	TimeIn = GetTimeCounter ();

	ContinuityBreak = FALSE;
	F = CommData.AlienFrame;
	if (wait_track == 0)
	{
		wait_track = which_track = (COUNT)~0;
		goto Rewind;
	}

	if (!(which_track = PlayingTrack ()))
	{
		// initial start of player
		if (wait_track == 1 || wait_track == (COUNT)~0)
		{
			ResumeTrack ();
			UnlockMutex (GraphicsLock);
			do
			{
				TaskSwitch ();
				LockMutex (GraphicsLock);
				which_track = PlayingTrack ();
				UnlockMutex (GraphicsLock);
			} while (!which_track);
			LockMutex (GraphicsLock);
		}
	}
	else if (which_track <= wait_track)
		ResumeTrack ();

	do
	{
		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			break;
		
		UnlockMutex (GraphicsLock);
		/* FIXME: is this a remnant of 128-tick clock?
		 * with 120-tick clock this will sleep for 1 tick --
		 * for 1/120th of a second; if ONE_SECOND is upgraded
		 * it will probably sleep for 2/120th of a second.
		 * Might need to be fixed.
		 */
		SleepThreadUntil (TimeIn + (ONE_SECOND / 64));
		TimeIn = GetTimeCounter ();
#if DEMO_MODE || CREATE_JOURNAL
		InputState = 0;
#else /* !(DEMO_MODE || CREATE_JOURNAL) */
		UpdateInputState ();
#endif

		LockMutex (GraphicsLock);
		if (PulsedInputState.key[KEY_MENU_CANCEL])
		{
			SetSliderImage (SetAbsFrameIndex (ActivityFrame, 8));
			JumpTrack ();
			CommData.AlienFrame = F;
			do_subtitles ((void *)~0);
			return (FALSE);
		}

		if (which_track)
		{
			BOOLEAN left=FALSE, right=FALSE;
			if (optSmoothScroll == OPT_PC)
			{
				left = PulsedInputState.key[KEY_MENU_LEFT];
				right = PulsedInputState.key[KEY_MENU_RIGHT];
			}
			else if (optSmoothScroll == OPT_3DO)
			{
				left = ImmediateInputState.key[KEY_MENU_LEFT];
				right = ImmediateInputState.key[KEY_MENU_RIGHT];
			}
			if (right)
			{
				SetSliderImage (SetAbsFrameIndex (ActivityFrame, 3));
				if (optSmoothScroll == OPT_PC && !FastForward_Page ())
				{
					SetSliderImage (SetAbsFrameIndex (ActivityFrame, 8));
					JumpTrack ();
					CommData.AlienFrame = F;
					do_subtitles ((void *)~0);
					return (FALSE);
				}
				else if (optSmoothScroll == OPT_3DO)
					FastForward_Smooth ();
				ContinuityBreak = TRUE;
				CommData.AlienFrame = 0;
			}
			else if (left)
			{
Rewind:
				SetSliderImage (SetAbsFrameIndex (ActivityFrame, 4));
				if (optSmoothScroll == OPT_PC)
					FastReverse_Page ();
				else if (optSmoothScroll == OPT_3DO)
					FastReverse_Smooth ();
				ContinuityBreak = TRUE;
				CommData.AlienFrame = 0;
			}
			else if (ContinuityBreak)
			{
				SetSliderImage (SetAbsFrameIndex (ActivityFrame, 2));
				if ((which_track = PlayingTrack ())
						&& which_track <= wait_track)
				{
					if (optSmoothScroll == OPT_3DO)
						ResumeTrack ();
				} 
				else
				{
					ContinuityBreak = FALSE;
					passed = (which_track != 0);
					break;
				}
				ContinuityBreak = FALSE;
			}
			else if (which_track == wait_track
					|| wait_track == (COUNT)~0)
				CommData.AlienFrame = F;
		}
	} while (ContinuityBreak
			|| ((which_track = PlayingTrack ()) && which_track <= wait_track));

	CommData.AlienFrame = F;
	do_subtitles ((void *)~0);

	if (wait_track == (COUNT)~0)
		SetSliderImage (SetAbsFrameIndex (ActivityFrame, 8));
	return (passed);
}

void
AlienTalkSegue (COUNT wait_track)
{
	BOOLEAN abort;
	if ((GLOBAL (CurrentActivity) & CHECK_ABORT)
			|| (CommData.AlienTransitionDesc.AnimFlags & TALK_INTRO))
		return;
	LockMutex (GraphicsLock);

	if (!pCurInputState->Initialized)
	{
		SetColorMap (GetColorMapAddress (CommData.AlienColorMap));
		DrawAlienFrame (CommData.AlienFrame, NULL_PTR);
		UpdateSpeechGraphics (TRUE);

		if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE
				|| (!GET_GAME_STATE (PLAYER_HYPNOTIZED)
				&& !GET_GAME_STATE (CHMMR_EMERGING)
				&& GET_GAME_STATE (CHMMR_BOMB_STATE) != 2
				&& (pMenuState == 0 || !GET_GAME_STATE (MOONBASE_ON_SHIP)
				|| GET_GAME_STATE (PROBE_ILWRATH_ENCOUNTER))))
		{
			RECT r;
	
			if (pMenuState == 0 && 
					LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
			{
				r.corner.x = SIS_ORG_X;
				r.corner.y = SIS_ORG_Y;
				r.extent.width = SIS_SCREEN_WIDTH;
				r.extent.height = SIS_SCREEN_HEIGHT;
				ScreenTransition (3, &r);
			}
			else
			{
				r.corner.x = 0;
				r.corner.y = 0;
				r.extent.width = SCREEN_WIDTH;
				r.extent.height = SCREEN_HEIGHT;
				ScreenTransition (3, &r);
			}
			UnbatchGraphics ();
		}
		else
		{
			BYTE clut_buf[] = {FadeAllToColor};
			
			UnbatchGraphics ();
			if (GET_GAME_STATE (MOONBASE_ON_SHIP)
					|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
				XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND * 2);
			else if (GET_GAME_STATE (CHMMR_EMERGING))
				XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND * 2);
			else
				XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND * 5);
		}
		pCurInputState->Initialized = TRUE;

		PlayMusic ((MUSIC_REF)CommData.AlienSong, TRUE, 1);
		SetMusicVolume (BACKGROUND_VOL);

		{
			DWORD TimeOut;

			TimeOut = GetTimeCounter () + (ONE_SECOND >> 1);
/* if (CommData.NumAnimations) */
				pCurInputState->AnimTask = AssignTask (ambient_anim_task,
						3072, "ambient animations");

			UnlockMutex (GraphicsLock);
			SleepThreadUntil (TimeOut);
			LockMutex (GraphicsLock);
		}

		LastActivity &= ~CHECK_LOAD;
	}
	if (wait_track == (COUNT)~0 || CommData.AlienTalkDesc.NumFrames)
	{
		if (!(CommData.AlienTransitionDesc.AnimFlags & TALK_INTRO))
		{
			CommData.AlienTransitionDesc.AnimFlags |= TALK_INTRO;
			if (CommData.AlienTransitionDesc.NumFrames)
				CommData.AlienTalkDesc.AnimFlags |= TALK_INTRO;
		}
					
		CommData.AlienTransitionDesc.AnimFlags &= ~PAUSE_TALKING;
		if (CommData.AlienTalkDesc.NumFrames)
			CommData.AlienTalkDesc.AnimFlags |= WAIT_TALKING;
		while (CommData.AlienTalkDesc.AnimFlags & TALK_INTRO)
		{
			UnlockMutex (GraphicsLock);
			TaskSwitch ();
			LockMutex (GraphicsLock);
		}
	}

	if (!(abort = SpewPhrases (wait_track)) || wait_track == (COUNT)~0)
		FadeMusic (FOREGROUND_VOL, ONE_SECOND);
	else
		CommData.AlienTransitionDesc.AnimFlags &= ~TALK_INTRO;

	if (wait_track == (COUNT)~0 || ! abort || CommData.AlienTalkDesc.NumFrames)
	{
		CommData.AlienTransitionDesc.AnimFlags |= TALK_DONE;
		if ((CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING))
			CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
	}

	UnlockMutex (GraphicsLock);
	FlushInput ();
	while (AnyButtonPress (TRUE))
		TaskSwitch ();

	do
		TaskSwitch ();
	while (CommData.AlienTalkDesc.AnimFlags & PAUSE_TALKING);
}

static BOOLEAN
DoCommunication (PENCOUNTER_STATE pES)
{
	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);

	if (!(CommData.AlienTransitionDesc.AnimFlags & (TALK_INTRO | TALK_DONE)))
	{
		AlienTalkSegue ((COUNT)~0);
	}

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		;
	else if (pES->num_responses == 0)
	{
		DWORD TimeIn, TimeOut;

		TimeOut = FadeMusic (0, ONE_SECOND * 3) + ONE_SECOND / 60;
		TimeIn = GetTimeCounter ();
		do
		{
			SleepThreadUntil (TimeIn + ONE_SECOND / 120);
			TimeIn = GetTimeCounter ();
			// Warning!  This used to re-gather input data to check for rewind.
			UpdateInputState ();
			if (PulsedInputState.key[KEY_MENU_LEFT])
			{
				FadeMusic (BACKGROUND_VOL, ONE_SECOND);
				LockMutex (GraphicsLock);
				CommData.AlienTransitionDesc.AnimFlags &= ~(TALK_INTRO | TALK_DONE);
				if (CommData.AlienTalkDesc.NumFrames)
				{
					if (!(CommData.AlienTransitionDesc.AnimFlags & TALK_INTRO))
					{
						CommData.AlienTransitionDesc.AnimFlags |= TALK_INTRO;
						if (CommData.AlienTransitionDesc.NumFrames)
							CommData.AlienTalkDesc.AnimFlags |= TALK_INTRO;
					}
					
					CommData.AlienTransitionDesc.AnimFlags &= ~PAUSE_TALKING;
					if (CommData.AlienTalkDesc.NumFrames)
						CommData.AlienTalkDesc.AnimFlags |= WAIT_TALKING;
					while (CommData.AlienTalkDesc.AnimFlags & TALK_INTRO)
					{
						UnlockMutex (GraphicsLock);
						TaskSwitch ();
						LockMutex (GraphicsLock);
					}
				}
				
				SpewPhrases (0);
				
				CommData.AlienTransitionDesc.AnimFlags |= TALK_DONE;
				if (CommData.AlienTalkDesc.NumFrames)
				{
					if ((CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING))
						CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
				}

				UnlockMutex (GraphicsLock);
				FlushInput ();
				while (AnyButtonPress (TRUE))
					TaskSwitch ();
				do
					TaskSwitch ();
				while (CommData.AlienTalkDesc.AnimFlags & PAUSE_TALKING);

				if (GLOBAL (CurrentActivity) & CHECK_ABORT)
					break;
				TimeOut = FadeMusic (0, ONE_SECOND * 2) + ONE_SECOND / 60;
				TimeIn = GetTimeCounter ();
			}
		} while (TimeIn <= TimeOut);
	}
	else
	{
		BYTE response;

		if (pES->top_response == (BYTE)~0)
		{
			pES->top_response = 0;
			LockMutex (GraphicsLock);
			RefreshResponses (pES);
			UnlockMutex (GraphicsLock);
		}

		if (PulsedInputState.key[KEY_MENU_SELECT])
		{
			pES->phrase_buf_index =
					pES->response_list[pES->cur_response].response_text.CharCount;
			wstrncpy (pES->phrase_buf,
					pES->response_list[pES->cur_response].response_text.pStr,
					pES->phrase_buf_index);
			pES->phrase_buf[pES->phrase_buf_index++] = '\0';

			LockMutex (GraphicsLock);
			FeedbackPlayerPhrase (pES->phrase_buf);
			StopTrack ();
			SetSliderImage (SetAbsFrameIndex (ActivityFrame, 2));
			UnlockMutex (GraphicsLock);

			FadeMusic (BACKGROUND_VOL, ONE_SECOND);

			CommData.AlienTransitionDesc.AnimFlags &= ~(TALK_INTRO | TALK_DONE);
			pES->num_responses = 0;
			(*pES->response_list[pES->cur_response].response_func)
					(pES->response_list[pES->cur_response].response_ref);
		}
		else if (PulsedInputState.key[KEY_MENU_CANCEL] && 
				LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
		{
			TFB_SoundChain *curr;
			RECT r;
			TEXT t;
#define DELTA_Y_SUMMARY 8
#define SUMMARY_CHARS (SIS_SCREEN_WIDTH - 34) / 4
#define MAX_SUMMARY_CHARS 52
#define MAX_COLS ((SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT - DELTA_Y_SUMMARY) / 8) - 1
			UNICODE buffer[MAX_SUMMARY_CHARS];
			UNICODE *temp;
			int i;
			int col = 0;
			DWORD CurTime;
			FONT fLast;

			LockMutex (GraphicsLock);
			DrawSISComWindow ();
			FeedbackPlayerPhrase (pES->phrase_buf);
			summary = TRUE;
			UnlockMutex (GraphicsLock);
			CurTime = GetTimeCounter ();
			SleepThreadUntil (CurTime + ONE_SECOND / 30);
			LockMutex (GraphicsLock);
			r.corner.x = 0;
			r.corner.y = 0;
			r.extent.width = SIS_SCREEN_WIDTH;
			r.extent.height = SIS_SCREEN_HEIGHT - SLIDER_Y - SLIDER_HEIGHT + 2;
			SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x05, 0x00), 0x6E));
			DrawFilledRectangle (&r);
			UnlockMutex (GraphicsLock);

			SetContextBackGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x05, 0x00), 0x6E));
			SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x6B));

			t.baseline.x = SAFE_X + 2;
			t.align = ALIGN_LEFT;
			t.baseline.y = SAFE_Y + DELTA_Y_SUMMARY;
			t.CharCount = (COUNT)~0;
			fLast = SetContextFont (TinyFont);

			for (curr = first_chain; curr != NULL; curr = curr->next)
			{
				temp = curr->text;
				if (temp == NULL)
					continue;
				// fprintf (stderr, "%s\n", temp);
				while (wstrlen (temp) > (unsigned int) SUMMARY_CHARS
						&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
				{
					int space_index;
					// find last space before it goes over the max chars per line
					for (i = SUMMARY_CHARS - 1; i > 0; i--)
					{
						if (temp[i] == ' ')
						{
							space_index = i;
							break;
						}
					}
					for (i = 0; i < space_index; i++, temp++)
					{
						buffer[i] = *temp;
					}
					buffer[i] = '\0';
					temp++;
					t.pStr = buffer;
					LockMutex (GraphicsLock);
					font_DrawText (&t);
					UnlockMutex (GraphicsLock);
					t.baseline.y += DELTA_Y_SUMMARY;
					col++;
					if (col > MAX_COLS)
					{
						t.baseline.x = SIS_SCREEN_WIDTH >> 1;
						t.align = ALIGN_CENTER;
						t.pStr = "_MORE_";
						SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x17, 0x00), 0x01));
						LockMutex (GraphicsLock);
						font_DrawText (&t);
						UnlockMutex (GraphicsLock);
						col = 0;
						t.baseline.x = SAFE_X + 2;
						t.align = ALIGN_LEFT;
						t.baseline.y = SAFE_Y + DELTA_Y_SUMMARY;

						WaitAnyButtonOrQuit (TRUE);

						LockMutex (GraphicsLock);
						SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x05, 0x00), 0x6E));
						DrawFilledRectangle (&r);
						UnlockMutex (GraphicsLock);
						SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x6B));
					}
				}
			
				if (GLOBAL (CurrentActivity) & CHECK_ABORT)
					break;				
				
				t.pStr = temp;
				LockMutex (GraphicsLock);
				font_DrawText (&t);
				UnlockMutex (GraphicsLock);
				t.baseline.y += DELTA_Y_SUMMARY;
				col++;
				if (col > MAX_COLS && curr->next != NULL)
				{
					
					t.baseline.x = SIS_SCREEN_WIDTH >> 1;
					t.align = ALIGN_CENTER;
					t.pStr = "_MORE_";
					SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x17, 0x00), 0x01));
					LockMutex (GraphicsLock);
					font_DrawText (&t);
					UnlockMutex (GraphicsLock);
					col = 0;
					t.baseline.x = SAFE_X + 2;
					t.align = ALIGN_LEFT;
					t.baseline.y = SAFE_Y + DELTA_Y_SUMMARY;

					WaitAnyButtonOrQuit (TRUE);

					LockMutex (GraphicsLock);
					SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x05, 0x00), 0x6E));
					DrawFilledRectangle (&r);
					UnlockMutex (GraphicsLock);
					SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x6B));
				}
			}

			WaitAnyButtonOrQuit (TRUE);

			LockMutex (GraphicsLock);
			SetContextFont (fLast);
			RefreshResponses (pES);
			SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));
			DrawFilledRectangle (&r);
			UnlockMutex (GraphicsLock);
			SummaryChange = TRUE;
			summary = FALSE;
			FlushInput ();
			while (AnyButtonPress (TRUE))
				TaskSwitch ();
		}
		else
		{
			response = pES->cur_response;
			if (PulsedInputState.key[KEY_MENU_LEFT])
			{
				FadeMusic (BACKGROUND_VOL, ONE_SECOND);
				LockMutex (GraphicsLock);
				FeedbackPlayerPhrase (pES->phrase_buf);

				CommData.AlienTransitionDesc.AnimFlags &= ~(TALK_INTRO | TALK_DONE);
				if (CommData.AlienTalkDesc.NumFrames)
				{
					if (!(CommData.AlienTransitionDesc.AnimFlags & TALK_INTRO))
					{
						CommData.AlienTransitionDesc.AnimFlags |= TALK_INTRO;
						if (CommData.AlienTransitionDesc.NumFrames)
							CommData.AlienTalkDesc.AnimFlags |= TALK_INTRO;
					}
					
					CommData.AlienTransitionDesc.AnimFlags &= ~PAUSE_TALKING;
					if (CommData.AlienTalkDesc.NumFrames)
						CommData.AlienTalkDesc.AnimFlags |= WAIT_TALKING;
					while (CommData.AlienTalkDesc.AnimFlags & TALK_INTRO)
					{
						UnlockMutex (GraphicsLock);
						TaskSwitch ();
						LockMutex (GraphicsLock);
					}
				}

				SpewPhrases (0);

				CommData.AlienTransitionDesc.AnimFlags |= TALK_DONE;
				if (CommData.AlienTalkDesc.NumFrames)
				{
					if ((CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING))
						CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
				}
				if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
				{
					RefreshResponses (pES);
					FadeMusic (FOREGROUND_VOL, ONE_SECOND);
				}
				UnlockMutex (GraphicsLock);
				FlushInput ();
				while (AnyButtonPress (TRUE))
					TaskSwitch ();
				do
					TaskSwitch ();
				while (CommData.AlienTalkDesc.AnimFlags & PAUSE_TALKING);
			}
			else if (PulsedInputState.key[KEY_MENU_UP])
				response = (BYTE)((response + (BYTE)(pES->num_responses - 1))
						% pES->num_responses);
			else if (PulsedInputState.key[KEY_MENU_DOWN])
				response = (BYTE)((BYTE)(response + 1)
						% pES->num_responses);

			if (response != pES->cur_response)
			{
				COORD y;

				LockMutex (GraphicsLock);
				BatchGraphics ();
				add_text (-2, &pES->response_list[pES->cur_response].response_text);

				pES->cur_response = response;

				y = add_text (-1, &pES->response_list[pES->cur_response].response_text);
				if (response < pES->top_response)
				{
					pES->top_response = 0;
					RefreshResponses (pES);
				}
				else if (y > SIS_SCREEN_HEIGHT)
				{
					pES->top_response = response;
					RefreshResponses (pES);
				}
				UnbatchGraphics ();
				UnlockMutex (GraphicsLock);
			}
		}
		
		return (TRUE);
	}

	LockMutex (GraphicsLock);

	if (pES->AnimTask)
	{
		UnlockMutex (GraphicsLock);
		ConcludeTask (pES->AnimTask);
		LockMutex (GraphicsLock);
		pES->AnimTask = 0;
	}
	CommData.AlienTransitionDesc.AnimFlags &= ~(TALK_INTRO | TALK_DONE);

	SetContext (SpaceContext);
	DestroyContext (ReleaseContext (TaskContext));
	TaskContext = 0;

	UnlockMutex (GraphicsLock);

	FlushPLUTXForms ();
	ColorChange = FALSE;
	ClearSubtitle = FALSE;

	FlushColorXForms ();
	StopMusic ();
	StopSound ();
	StopTrack ();
	SleepThreadUntil (FadeMusic (FOREGROUND_VOL, 0) + ONE_SECOND / 60);

	return (FALSE);
}

void
DoResponsePhrase (RESPONSE_REF R, RESPONSE_FUNC response_func,
		UNICODE *ConstructStr)
{
	PENCOUNTER_STATE pES = pCurInputState;
	PRESPONSE_ENTRY pEntry;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
			
	if (pES->num_responses == 0)
	{
		pES->cur_response = 0;
		pES->top_response = (BYTE)~0;
	}

	pEntry = &pES->response_list[pES->num_responses];
	pEntry->response_ref = R;
	pEntry->response_text.pStr = ConstructStr;
	if (pEntry->response_text.pStr)
		pEntry->response_text.CharCount = wstrlen (ConstructStr);
	else
	{
		STRING locString;
		
		locString = SetAbsStringTableIndex (CommData.ConversationPhrases,
				(COUNT) (R - 1));
		pEntry->response_text.pStr =
				(UNICODE *) GetStringAddress (locString);
		pEntry->response_text.CharCount = GetStringLength (locString);
//#define BVT_PROBLEM
#ifdef BVT_PROBLEM
		if (pEntry->response_text.pStr[pEntry->response_text.CharCount - 1]
				== '\0')
			--pEntry->response_text.CharCount;
#endif /* BVT_PROBLEM */
	}
	pEntry->response_func = response_func;
	++pES->num_responses;
}

static void
HailAlien (void)
{
	MEM_HANDLE hOldIndex;
	ENCOUNTER_STATE ES;
	FONT PlayerFont, OldFont;

	pCurInputState = &ES;
	memset (pCurInputState, 0, sizeof (*pCurInputState));

	ES.InputFunc = DoCommunication;
	hOldIndex = SetResourceIndex (hResIndex);
	PlayerFont = CaptureFont ((FONT_REF)LoadGraphic (PLAYER_FONT));
	SetResourceIndex (hOldIndex);

	CommData.AlienFrame = CaptureDrawable (
			LoadGraphicInstance ((RESOURCE)CommData.AlienFrame)
			);
	CommData.AlienFont = CaptureFont ((FONT_REF)
			LoadGraphic ((RESOURCE)CommData.AlienFont)
			);
	CommData.AlienColorMap = CaptureColorMap (
			LoadColorMapInstance ((RESOURCE)CommData.AlienColorMap)
			);
	CommData.AlienSong = LoadMusicInstance ((RESOURCE)CommData.AlienSong);
	CommData.ConversationPhrases = CaptureStringTable (
			LoadStringTableInstance ((RESOURCE)CommData.ConversationPhrases)
			);

	ES.phrase_buf_index = 1;
	ES.phrase_buf[0] = '\0';

	SetContext (SpaceContext);
	OldFont = SetContextFont (PlayerFont);

	{
		RECT r;

		TaskContext = CaptureContext (CreateContext ());
		SetContext (TaskContext);
		SetContextFGFrame (Screen);
		GetFrameRect (CommData.AlienFrame, &r);
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;

		SetTransitionSource (NULL);
		BatchGraphics ();
		if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
		{
			r.corner.x = (SCREEN_WIDTH - SIS_SCREEN_WIDTH) >> 1;
			SetContextClipRect (&r);
		}
		else
		{
			r.corner.x = SIS_ORG_X;
			SetContextClipRect (&r);

			if (pMenuState == 0)
			{
				RepairSISBorder ();
				UnlockMutex (GraphicsLock);
				DrawMenuStateStrings ((BYTE)~0, 1);
				LockMutex (GraphicsLock);
			}
			else /* in starbase */
			{
				UnlockMutex (GraphicsLock);
				DrawSISFrame ();
				LockMutex (GraphicsLock);
				if (GET_GAME_STATE (STARBASE_AVAILABLE))
				{
					DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 1));
					DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE + 0));
				}
				else
				{
					DrawSISMessage (NULL_PTR);
					DrawSISTitle (GLOBAL_SIS (PlanetName));
				}
			}
		}

		DrawSISComWindow ();
	}

	UnlockMutex (GraphicsLock);

	LastActivity |= CHECK_LOAD; /* prevent spurious input */
	(*CommData.init_encounter_func) ();
	DoInput ((PVOID)&ES, FALSE);
	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
		(*CommData.uninit_encounter_func) ();

	LockMutex (GraphicsLock);

	DestroyStringTable (ReleaseStringTable (CommData.ConversationPhrases));
	DestroyMusic ((MUSIC_REF)CommData.AlienSong);
	DestroyColorMap (ReleaseColorMap (CommData.AlienColorMap));
	DestroyFont (ReleaseFont (CommData.AlienFont));
	DestroyDrawable (ReleaseDrawable (CommData.AlienFrame));

	SetContext (SpaceContext);
	SetContextFont (OldFont);
	DestroyFont (ReleaseFont (PlayerFont));

	CommData.ConversationPhrases = 0;
	pCurInputState = 0;
}

COUNT
InitCommunication (RESOURCE which_comm)
{
	COUNT status;
	MEM_HANDLE hOldIndex, hIndex;
	LOCDATAPTR LocDataPtr;

	last_subtitle = NULL;

	LockMutex (GraphicsLock);

	if (LastActivity & CHECK_LOAD)
	{
		LastActivity &= ~CHECK_LOAD;
		if (which_comm != COMMANDER_CONVERSATION)
		{
			if (LOBYTE (LastActivity) == 0)
			{
				UnlockMutex (GraphicsLock);
				DrawSISFrame ();
				LockMutex (GraphicsLock);
			}
			else
			{
				ClearSISRect (DRAW_SIS_DISPLAY);
				RepairSISBorder ();
			}
			DrawSISMessage (NULL_PTR);
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
				DrawHyperCoords (
						GLOBAL (ShipStamp.origin)
						);
			else if (HIWORD (GLOBAL (ShipStamp.frame)) == 0)
				DrawHyperCoords (CurStarDescPtr->star_pt);
			else
				DrawSISTitle (GLOBAL_SIS (PlanetName));
		}
	}

	if (which_comm == 0)
		status = URQUAN_PROBE_SHIP, which_comm = URQUAN_CONVERSATION;
	else
	{
		if (which_comm == (RESOURCE) YEHAT_REBEL_CONVERSATION)
			status = YEHAT_REBEL_SHIP, which_comm = YEHAT_CONVERSATION;
		else if (((status = GET_PACKAGE (which_comm)
				- GET_PACKAGE (ARILOU_CONVERSATION)
				+ ARILOU_SHIP) >= YEHAT_REBEL_SHIP))
			status = HUMAN_SHIP; /* conversation exception, set to self */
		ActivateStarShip (status, SPHERE_TRACKING);

		if (which_comm == ORZ_CONVERSATION
				|| (which_comm == TALKING_PET_CONVERSATION
				&& (!GET_GAME_STATE (TALKING_PET_ON_SHIP)
				|| LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE))
				|| (which_comm != CHMMR_CONVERSATION
				&& which_comm != SYREEN_CONVERSATION
				))//&& (ActivateStarShip (status, CHECK_ALLIANCE) & BAD_GUY)))
			BuildBattle (1);
	}

	hOldIndex = SetResourceIndex (hResIndex);
	if ((hIndex = OpenResourceIndexInstance (which_comm)) == 0)
	{
		SET_GAME_STATE (BATTLE_SEGUE, 1);
		LocDataPtr = 0;
	}
	else
	{
		SetResourceIndex (hIndex);

		LocDataPtr = (LOCDATAPTR)init_race (
						    status != YEHAT_REBEL_SHIP ? which_comm : (RESOURCE)YEHAT_REBEL_CONVERSATION
				);
		if (LocDataPtr)
			CommData = *LocDataPtr;
	}

	UnlockMutex (GraphicsLock);

	if (GET_GAME_STATE (BATTLE_SEGUE) == 0)
	{
		// Not offered the chance to attack.
		status = HAIL;
	}
	else if ((status = InitEncounter ()) == HAIL && LocDataPtr)
	{
		// The player chose to talk.
		SET_GAME_STATE (BATTLE_SEGUE, 0);
	}
	else
	{
		// The player chose to attack.
		status = ATTACK;
		SET_GAME_STATE (BATTLE_SEGUE, 1);
	}

	LockMutex (GraphicsLock);

	if (status == HAIL)
	{
		cur_comm = which_comm;
		HailAlien ();
		cur_comm = 0;
	}
	else if (LocDataPtr)
		(*CommData.uninit_encounter_func) ();

	SetResourceIndex (hOldIndex);
	CloseResourceIndex (hIndex);

	UnlockMutex (GraphicsLock);

	status = 0;
	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE
				&& (GLOBAL (glob_flags) & CYBORG_ENABLED))
			ReinitQueue (&GLOBAL (npc_built_ship_q));

		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 0);
		status = (GET_GAME_STATE (BATTLE_SEGUE)
				&& GetHeadLink (&GLOBAL (npc_built_ship_q)));
		if (status)
		{
			// Start combat
			BuildBattle (0);
			EncounterBattle ();
		}
		else
		{
			SET_GAME_STATE (BATTLE_SEGUE, 0);
		}
	}

	UninitEncounter ();

	return (status);
}

void
RaceCommunication (void)
{
	COUNT i, status;
	HSTARSHIP hStarShip;
	SHIP_FRAGMENTPTR FragPtr;
	RESOURCE RaceComm[] =
	{
		RACE_COMMUNICATION
	};
	extern ACTIVITY NextActivity;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
	{
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		CloneShipFragment (SAMATRA_SHIP, &GLOBAL (npc_built_ship_q), 0);
		InitCommunication (TALKING_PET_CONVERSATION);
		if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
		{
			GLOBAL (CurrentActivity) = WON_LAST_BATTLE;
		}
		return;
	}
		/* going into talking pet conversation */
	else if (NextActivity & CHECK_LOAD)
	{
		BYTE ec;

		ec = GET_GAME_STATE (ESCAPE_COUNTER);

		if (GET_GAME_STATE (FOUND_PLUTO_SPATHI) == 1)
			InitCommunication (SPATHI_CONVERSATION);
		else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == 0)
			InitCommunication (TALKING_PET_CONVERSATION);
		else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & ((1 << 4) | (1 << 5)))
			InitCommunication (ILWRATH_CONVERSATION);
		else
			InitCommunication (CHMMR_CONVERSATION);
		 if (GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
		{
			NextActivity = GLOBAL (CurrentActivity) & ~START_ENCOUNTER;
			if (LOBYTE (NextActivity) == IN_INTERPLANETARY)
				NextActivity |= START_INTERPLANETARY;
			GLOBAL (CurrentActivity) |= CHECK_LOAD; /* fake a load game */
		}

		SET_GAME_STATE (ESCAPE_COUNTER, ec);
		return;
	}
	else if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
	{
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) >= 2)
		{
			InitCommunication (ARILOU_CONVERSATION);
			return;
		}
		else
		{
			COUNT NumShips;
			HENCOUNTER hEncounter;
			ENCOUNTERPTR EncounterPtr;

			hEncounter = GetHeadEncounter ();
			LockEncounter (hEncounter, &EncounterPtr);

			NumShips = LONIBBLE (EncounterPtr->SD.Index);
			for (i = 0; i < NumShips; ++i)
			{
				CloneShipFragment (
						EncounterPtr->SD.Type,
						&GLOBAL (npc_built_ship_q), 0
						);
			}

			CurStarDescPtr = (STAR_DESCPTR)&EncounterPtr->SD;
			UnlockEncounter (hEncounter);
		}
	}

	hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
	FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (npc_built_ship_q), hStarShip
			);
	i = GET_RACE_ID (FragPtr);
	UnlockStarShip (
			&GLOBAL (npc_built_ship_q), hStarShip
			);

   status = InitCommunication (RaceComm[i]);

	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
		return;

	if (i == CHMMR_SHIP)
		ReinitQueue (&GLOBAL (npc_built_ship_q));

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
	{
				/* if used destruct code in interplanetary */
		if (i == SLYLANDRO_SHIP && status == 0)
			ReinitQueue (&GLOBAL (npc_built_ship_q));
	}
	else
	{
		PEXTENDED_STAR_DESC pESD;

		pESD = (PEXTENDED_STAR_DESC)CurStarDescPtr;
		if (pESD)
		{
			BYTE i, NumShips;

			NumShips = (BYTE)CountLinks (&GLOBAL (npc_built_ship_q));
			pESD->Index = MAKE_BYTE (
					NumShips, HINIBBLE (pESD->Index)
					);
			pESD->Index |= ENCOUNTER_REFORMING;
			if (status == 0)
				pESD->Index |= ONE_SHOT_ENCOUNTER;

			for (i = 0; i < NumShips; ++i)
			{
				HSTARSHIP hStarShip;
				SHIP_FRAGMENTPTR TemplatePtr;

				hStarShip = GetStarShipFromIndex (
						&GLOBAL (npc_built_ship_q), i
						);
				TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (npc_built_ship_q), hStarShip
						);
				pESD->ShipList[i] = TemplatePtr->ShipInfo;
				pESD->ShipList[i].var1 = GET_RACE_ID (TemplatePtr);
				UnlockStarShip (
						&GLOBAL (npc_built_ship_q), hStarShip
						);
			}

			ReinitQueue (&GLOBAL (npc_built_ship_q));
			CurStarDescPtr = 0;
		}
	}
}

int
do_subtitles (UNICODE *pStr)
{
	static UNICODE *last_page = NULL;
	LockMutex (subtitle_mutex);
	if (pStr == 0)
	{
		subtitle_state = DONE_SUBTITLE;
		UnlockMutex (subtitle_mutex);
		return (subtitle_state);
	}
	else if (pStr == (void *)~0)
	{
		subtitle_state = WAIT_SUBTITLE;
	}
	else
	{
		if (last_page == pStr)
		{
			UnlockMutex (subtitle_mutex);
			return (subtitle_state);
		}
		subtitle_state = READ_SUBTITLE;
		ClearSubtitle = TRUE;
//		fprintf (stderr, "changed page to: %d\n", cur_page);
	}
	last_page = pStr;

	switch (subtitle_state)
	{
		case READ_SUBTITLE:
		{
			TEXT t;
			
			CommData.AlienTextTemplate.pStr = pStr;
			t = CommData.AlienTextTemplate;

			CommData.AlienTextTemplate.CharCount = t.pStr - CommData.AlienTextTemplate.pStr;
			subtitle_state = WAIT_SUBTITLE;
			break;
		}
		case WAIT_SUBTITLE:
		{
			subtitle_state = DONE_SUBTITLE;
			ClearSubtitle = TRUE;
		}
		case DONE_SUBTITLE:
			break;
	}
	UnlockMutex (subtitle_mutex);

	return (subtitle_state);
}
