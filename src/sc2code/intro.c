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

#include <ctype.h>
#include "starcon.h"
#include "libs/vidlib.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound.h"

//Added by Chris

BOOLEAN InitGameKernel (void);

void LoadMasterShipList (void);

//End Added by Chris

typedef struct
{
	/* standard state required by DoInput */
	BOOLEAN (*InputFunc) (PVOID pInputState);
	COUNT MenuRepeatDelay;

	/* Presentation state */
	TimeCount StartTime;
	TimeCount LastSyncTime;
	TimeCount TimeOut;
	int TimeOutOnSkip;
	STRING SlideShow;
	FONT Font;
	FRAME Frame;
	MUSIC_REF MusicRef;
	COUNT OperIndex;
	COLOR TextFadeColor;
	COLOR TextColor;
	COLOR TextBackColor;
	int TextVPos;
	RECT clip_r;
	RECT tfade_r;
#define MAX_TEXT_LINES 15
	TEXT TextLines[MAX_TEXT_LINES];
	COUNT LinesCount;
	char Buffer[512];

} PRESENTATION_INPUT_STATE, *PPRESENTATION_INPUT_STATE;

BOOLEAN
DoFMV (char *name, char *loopname, BOOLEAN uninit)
{
	VIDEO_REF VidRef;

	VidRef = LoadVideoFile (name);
	if (!VidRef)
		return FALSE;
	VidPlay (VidRef, loopname, uninit);
	VidDoInput ();
	VidStop ();
	DestroyVideo (VidRef);
	
	return TRUE;
}

static void
CopyTextString (char *Buffer, COUNT BufSize, const char *Src)
{
	strncpy (Buffer, Src, BufSize);
	Buffer[BufSize - 1] = '\0';
}

static BOOLEAN
ParseColorString (const char *Src, COLOR* pColor)
{
	unsigned clr;
	if (1 != sscanf (Src, "%x", &clr))
		return FALSE;

	*pColor = BUILD_COLOR (MAKE_RGB15 (
			(clr >> 19) & 0x1f, (clr >> 11) & 0x1f, (clr >> 3) & 0x1f),
			0);
	return TRUE;
}

static BOOLEAN
DoFadeScreen (PRESENTATION_INPUT_STATE* pPIS, const char *Src, BYTE FadeType)
{
	BYTE xform_buf[1] = {FadeType};
	int msecs;
	if (1 == sscanf (Src, "%d", &msecs))
	{
		pPIS->TimeOut = XFormColorMap (
				(COLORMAPPTR) xform_buf,
				(SIZE)(msecs * ONE_SECOND / 1000)) + ONE_SECOND / 10;
		pPIS->TimeOutOnSkip = FALSE;
	}
	return TRUE;
}

static void
DrawTracedText (TEXT *pText, COLOR Fore, COLOR Back)
{
	SetContextForeGroundColor (Back);
	pText->baseline.x--;
	font_DrawText (pText);
	pText->baseline.x += 2;
	font_DrawText (pText);
	pText->baseline.x--;
	pText->baseline.y--;
	font_DrawText (pText);
	pText->baseline.y += 2;
	font_DrawText (pText);
	pText->baseline.y--;
	SetContextForeGroundColor (Fore);
	font_DrawText (pText);
}

static COUNT
ParseTextLines (TEXT *Lines, COUNT MaxLines, char* Buffer)
{
	COUNT i;
	const char* pEnd = Buffer + strlen(Buffer);

	for (i = 0; i < MaxLines && Buffer < pEnd; ++i, ++Lines)
	{
		char* pTerm = strchr(Buffer, '\n');
		if (!pTerm)
			pTerm = Buffer + strlen(Buffer);
		*pTerm = '\0'; /* terminate string */
		Lines->pStr = Buffer;
		Lines->CharCount = ~0;
		Buffer = pTerm + 1;
	}
	return i;
}

BOOLEAN
DoPresentation (PVOID pIS)
{
	PRESENTATION_INPUT_STATE* pPIS = (PRESENTATION_INPUT_STATE*) pIS;

	if (PulsedInputState.key[KEY_MENU_CANCEL]
			|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
		return FALSE; /* abort requested - we are done */

	if (pPIS->TimeOut)
	{
		if (GetTimeCounter () > pPIS->TimeOut)
		{	/* time elapsed - continue normal ops */
			pPIS->TimeOut = 0;
			return TRUE;
		}
		
		if (pPIS->TimeOutOnSkip &&
			(PulsedInputState.key[KEY_MENU_SELECT]
			|| PulsedInputState.key[KEY_MENU_SPECIAL]
			|| PulsedInputState.key[KEY_MENU_RIGHT]) )
		{	/* skip requested - continue normal ops */
			pPIS->TimeOut = 0;
			return TRUE;
		}

		SleepThread (ONE_SECOND / 20);
		return TRUE;
	}

	while (pPIS->OperIndex < GetStringTableCount (pPIS->SlideShow))
	{
		char Opcode[16];

		STRINGPTR pStr = GetStringAddress (pPIS->SlideShow);
		pPIS->OperIndex++;
		pPIS->SlideShow = SetRelStringTableIndex (pPIS->SlideShow, 1);

		if (!pStr)
			continue;
		if (1 != sscanf (pStr, "%15s", Opcode))
			continue;
		pStr += strlen(Opcode);
		if (*pStr != '\0')
			++pStr;
		strupr(Opcode);

		if (strcmp (Opcode, "DIMS") == 0)
		{	/* set dimensions */
			int w, h;
			if (2 == sscanf (pStr, "%d %d", &w, &h))
			{
				pPIS->clip_r.extent.width = w;
				pPIS->clip_r.extent.height = h;
				/* center on screen */
				pPIS->clip_r.corner.x = (SCREEN_WIDTH - w) / 2;
				pPIS->clip_r.corner.y = (SCREEN_HEIGHT - h) / 2;
				LockMutex (GraphicsLock);
				SetContextClipRect (&pPIS->clip_r);
				UnlockMutex (GraphicsLock);
			}
		}
		if (strcmp (Opcode, "FONT") == 0)
		{	/* set font */
			CopyTextString (pPIS->Buffer, sizeof(pPIS->Buffer), pStr);
			if (pPIS->Font)
				DestroyFont (ReleaseFont (pPIS->Font));
			pPIS->Font = CaptureFont ((FONT_REF) LoadFontFile (pPIS->Buffer));

			LockMutex (GraphicsLock);
			SetContextFont (pPIS->Font);
			UnlockMutex (GraphicsLock);
		}
		else if (strcmp (Opcode, "ANI") == 0)
		{	/* set ani */
			CopyTextString (pPIS->Buffer, sizeof(pPIS->Buffer), pStr);
			if (pPIS->Frame)
				DestroyDrawable (ReleaseDrawable (pPIS->Frame));
			pPIS->Frame = CaptureDrawable (LoadCelFile (pPIS->Buffer));
		}
		else if (strcmp (Opcode, "MUSIC") == 0)
		{	/* set music */
			CopyTextString (pPIS->Buffer, sizeof(pPIS->Buffer), pStr);
			if (pPIS->MusicRef)
			{
				StopMusic ();
				DestroyMusic (pPIS->MusicRef);
			}
			pPIS->MusicRef = LoadMusicFile (pPIS->Buffer);
			PlayMusic (pPIS->MusicRef, FALSE, 1);
		}
		else if (strcmp (Opcode, "WAIT") == 0)
		{	/* wait */
			int msecs;
			if (1 == sscanf (pStr, "%d", &msecs))
			{
				pPIS->TimeOut = GetTimeCounter ()
						+ msecs * ONE_SECOND / 1000;
				pPIS->TimeOutOnSkip = TRUE;
				return TRUE;
			}
		}
		else if (strcmp (Opcode, "SYNC") == 0)
		{	/* absolute time-sync */
			int msecs;
			if (1 == sscanf (pStr, "%d", &msecs))
			{
				pPIS->LastSyncTime = pPIS->StartTime
						+ msecs * ONE_SECOND / 1000;
				pPIS->TimeOut = pPIS->LastSyncTime;
				pPIS->TimeOutOnSkip = FALSE;
				return TRUE;
			}
		}
		else if (strcmp (Opcode, "RESYNC") == 0)
		{	/* flush and update absolute sync point */
			pPIS->LastSyncTime = pPIS->StartTime = GetTimeCounter ();
		}
		else if (strcmp (Opcode, "DSYNC") == 0)
		{	/* delta time-sync; from the last absolute sync */
			int msecs;
			if (1 == sscanf (pStr, "%d", &msecs))
			{
				pPIS->TimeOut = pPIS->LastSyncTime
						+ msecs * ONE_SECOND / 1000;
				pPIS->TimeOutOnSkip = FALSE;
				return TRUE;
			}
		}
		else if (strcmp (Opcode, "TC") == 0)
		{	/* text fore color */
			ParseColorString (pStr, &pPIS->TextColor);
		}
		else if (strcmp (Opcode, "TBC") == 0)
		{	/* text back color */
			ParseColorString (pStr, &pPIS->TextBackColor);
		}
		else if (strcmp (Opcode, "TFC") == 0)
		{	/* text fade color */
			ParseColorString (pStr, &pPIS->TextFadeColor);
		}
		else if (strcmp (Opcode, "TVA") == 0)
		{	/* text vertical align */
			pPIS->TextVPos = toupper (*pStr);
		}
		else if (strcmp (Opcode, "TEXT") == 0)
		{	/* simple text draw */
			SIZE leading;
			COUNT i;
			COORD y;
			
			CopyTextString (pPIS->Buffer, sizeof(pPIS->Buffer), pStr);
			pPIS->LinesCount = ParseTextLines (pPIS->TextLines,
					MAX_TEXT_LINES, pPIS->Buffer);
			
			LockMutex (GraphicsLock);
			GetContextFontLeading (&leading);
			UnlockMutex (GraphicsLock);
			switch (pPIS->TextVPos)
			{
			case 'T': /* top */
				y = leading;
				break;
			case 'M': /* middle */
				y = (pPIS->clip_r.extent.height
						- pPIS->LinesCount * leading) / 2;
				break;
			default: /* bottom */
				y = pPIS->clip_r.extent.height - pPIS->LinesCount * leading;
			}
			for (i = 0; i < pPIS->LinesCount; ++i, y += leading)
			{
				pPIS->TextLines[i].align = ALIGN_CENTER;
				pPIS->TextLines[i].valign = VALIGN_MIDDLE;
				pPIS->TextLines[i].baseline.x = SCREEN_WIDTH / 2;
				pPIS->TextLines[i].baseline.y = y;
			}

			LockMutex (GraphicsLock);
			SetContextClipping (TRUE);
			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTracedText (pPIS->TextLines + i,
						pPIS->TextColor, pPIS->TextBackColor);
			UnlockMutex (GraphicsLock);
		}
		else if (strcmp (Opcode, "TFI") == 0)
		{	/* text fade-in */
			SIZE leading;
			COUNT i;
			COORD y;
			
			CopyTextString (pPIS->Buffer, sizeof(pPIS->Buffer), pStr);
			pPIS->LinesCount = ParseTextLines (pPIS->TextLines,
					MAX_TEXT_LINES, pPIS->Buffer);
			
			LockMutex (GraphicsLock);
			GetContextFontLeading (&leading);
			UnlockMutex (GraphicsLock);

			switch (pPIS->TextVPos)
			{
			case 'T': /* top */
				y = leading;
				break;
			case 'M': /* middle */
				y = (pPIS->clip_r.extent.height
						- pPIS->LinesCount * leading) / 2;
				break;
			default: /* bottom */
				y = pPIS->clip_r.extent.height - pPIS->LinesCount * leading;
			}
			pPIS->tfade_r = pPIS->clip_r;
			pPIS->tfade_r.corner.y += y - leading;
			pPIS->tfade_r.extent.height = (pPIS->LinesCount + 1) * leading;
			for (i = 0; i < pPIS->LinesCount; ++i, y += leading)
			{
				pPIS->TextLines[i].align = ALIGN_CENTER;
				pPIS->TextLines[i].valign = VALIGN_MIDDLE;
				pPIS->TextLines[i].baseline.x = SCREEN_WIDTH / 2;
				pPIS->TextLines[i].baseline.y = y;
			}

			LockMutex (GraphicsLock);
			SetContextClipping (TRUE);
			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTracedText (pPIS->TextLines + i,
						pPIS->TextFadeColor, pPIS->TextFadeColor);

			/* do transition */
			SetTransitionSource (&pPIS->tfade_r);
			BatchGraphics ();
			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTracedText (pPIS->TextLines + i,
						pPIS->TextColor, pPIS->TextBackColor);
			ScreenTransition (3, &pPIS->tfade_r);
			UnbatchGraphics ();
			
			UnlockMutex (GraphicsLock);
		}
		else if (strcmp (Opcode, "TFO") == 0)
		{	/* text fade-out */
			COUNT i;
			
			LockMutex (GraphicsLock);
			SetContextClipping (TRUE);
			/* do transition */
			SetTransitionSource (&pPIS->tfade_r);
			BatchGraphics ();
			for (i = 0; i < pPIS->LinesCount; ++i)
				DrawTracedText (pPIS->TextLines + i,
						pPIS->TextFadeColor, pPIS->TextFadeColor);
			ScreenTransition (3, &pPIS->tfade_r);
			UnbatchGraphics ();
			UnlockMutex (GraphicsLock);
		}
		else if (strcmp (Opcode, "DRAW") == 0)
		{	/* draw a graphic */
			int index;
			if (1 == sscanf (pStr, "%d", &index))
			{
				STAMP s;

				s.frame = SetAbsFrameIndex (pPIS->Frame, (COUNT)index);
				s.origin.x = 0;
				s.origin.y = 0;
				LockMutex (GraphicsLock);
				SetContextClipping (TRUE);
				DrawStamp (&s);
				UnlockMutex (GraphicsLock);
			}
		}
		else if (strcmp (Opcode, "FTC") == 0)
		{	/* fade to color */
			return DoFadeScreen (pPIS, pStr, FadeAllToColor);
		}
		else if (strcmp (Opcode, "FTB") == 0)
		{	/* fade to black */
			return DoFadeScreen (pPIS, pStr, FadeAllToBlack);
		}
		else if (strcmp (Opcode, "FTW") == 0)
		{	/* fade to white */
			return DoFadeScreen (pPIS, pStr, FadeAllToWhite);
		}
		else if (strcmp (Opcode, "NOOP") == 0)
		{	/* no operation - must be a comment in script */
			/* do nothing */
		}
	}
	/* we are all done */
	return FALSE;
}

BOOLEAN
ShowPresentation (char *name)
{
	CONTEXT OldContext;
	FONT OldFont;
	RECT OldRect;
	PRESENTATION_INPUT_STATE pis;
	RECT r = {{0, 0}, {SCREEN_WIDTH, SCREEN_HEIGHT}};

	pis.SlideShow = CaptureStringTable (
			LoadStringTableFile (contentDir, name)
			);
	if (!pis.SlideShow)
		return FALSE;
	pis.SlideShow = SetAbsStringTableIndex (pis.SlideShow, 0);
	pis.OperIndex = 0;

	LockMutex (GraphicsLock);
	OldContext = SetContext (ScreenContext);
	GetContextClipRect (&OldRect);
	OldFont = SetContextFont (NULL);
	/* paint black rect over screen	*/
	SetContextForeGroundColor (BUILD_COLOR (
			MAKE_RGB15 (0x0, 0x0, 0x0), 0x00));
	DrawFilledRectangle (&r);	
	UnlockMutex (GraphicsLock);

	FlushInput ();
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	pis.MenuRepeatDelay = 0;
	pis.InputFunc = DoPresentation;
	pis.Font = 0;
	pis.Frame = 0;
	pis.MusicRef = 0;
	pis.TextVPos = 'B';
	pis.LastSyncTime = pis.StartTime = GetTimeCounter ();
	pis.TimeOut = 0;
	DoInput (&pis, TRUE);

	SleepThreadUntil (FadeMusic (0, ONE_SECOND));
	StopMusic ();
	FadeMusic (MAX_VOLUME, 0);

	DestroyMusic (pis.MusicRef);
	DestroyDrawable (ReleaseDrawable (pis.Frame));
	DestroyFont (ReleaseFont (pis.Font));
	DestroyStringTable (ReleaseStringTable (pis.SlideShow));

	LockMutex (GraphicsLock);
	SetContextFont (OldFont);
	SetContextClipRect (&OldRect);
	SetContext (OldContext);
	UnlockMutex (GraphicsLock);

	return TRUE;
}

/* //Already defined in fmv.c
void
Introduction (void)
{
	BYTE xform_buf[1];
	STAMP s;
	DWORD TimeOut;
	BOOLEAN InputState;

	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap (
			(COLORMAPPTR)xform_buf, ONE_SECOND / 120));
	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	s.origin.x = s.origin.y = 0;
	s.frame = CaptureDrawable (LoadGraphic (TITLE_ANIM));
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));
	UnlockMutex (GraphicsLock);

	FlushInput ();

	xform_buf[0] = FadeAllToColor;
	TimeOut = XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2);
	LoadMasterShipList ();
	SleepThreadUntil (TimeOut);
	
	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	TimeOut += ONE_SECOND * 3;
	while (!(InputState = AnyButtonPress (FALSE)) && TaskSwitch () <= TimeOut)
		;
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2));

	if (InputState == 0)
		DoFMV ("intro", NULL, FALSE);
	
	InitGameKernel ();
}
*/

/* //Already defined in fmv.c
void
Victory (void)
{
	BYTE xform_buf[1];

	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2));

	DoFMV ("victory", NULL, TRUE);
		
	InitGameKernel ();
}
*/



