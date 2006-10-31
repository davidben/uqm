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

#include "credits.h"

#include "controls.h"
#include "encount.h"
#include "options.h"
#include "oscill.h"
#include "comm.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "sounds.h"
#include "setup.h"
#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound.h"
#include <math.h>

// Rates in pixel lines per second
#define CREDITS_BASE_RATE   9
#define CREDITS_MAX_RATE    130
// Maximum frame rate
#define CREDITS_FRAME_RATE  36

#define CREDITS_TIMEOUT   (ONE_SECOND * 5)

static volatile int CreditsRate;
static volatile BOOLEAN OutTakesRunning;
static volatile BOOLEAN CreditsRunning;
static STRING CreditsTab;
static FRAME CreditsBack;

typedef struct
{
	int size;
	DWORD res;
	FONT font;
} FONT_SIZE_DEF;

static FONT_SIZE_DEF CreditsFont[] =
{
	{ 13, PT13AA_FONT, 0 },
	{ 17, PT17AA_FONT, 0 },
	{ 45, PT45AA_FONT, 0 },
	{  0,           0, 0 },
};


static FRAME
Credits_MakeTextFrame (int w, int h, COLOR TransColor)
{
	FRAME OldFrame;
	FRAME f;

	f = CaptureDrawable (CreateDrawable (WANT_PIXMAP, w, h, 1));
	SetFrameTransparentColor (f, TransColor);

	OldFrame = SetContextFGFrame (f);
	SetContextBackGroundColor (TransColor);
	ClearDrawable ();
	SetContextFGFrame (OldFrame);

	return f;
}

static int
ParseTextLines (TEXT *Lines, int MaxLines, char *Buffer)
{
	int i;
	const char* pEnd = Buffer + strlen (Buffer);

	for (i = 0; i < MaxLines && Buffer < pEnd; ++i, ++Lines)
	{
		char* pTerm = strchr (Buffer, '\n');
		if (!pTerm)
			pTerm = Buffer + strlen (Buffer);
		*pTerm = '\0'; /* terminate string */
		Lines->pStr = Buffer;
		Lines->CharCount = ~0;
		Buffer = pTerm + 1;
	}
	return i;
}

#define MAX_TEXT_LINES 50
#define MAX_TEXT_COLS  5

static FRAME
Credits_RenderTextFrame (CONTEXT TempContext, int *istr, int dir,
		COLOR BackColor, COLOR ForeColor)
{
	FRAME f;
	CONTEXT OldContext;
	FRAME OldFrame;
	TEXT TextLines[MAX_TEXT_LINES];
	STRINGPTR pStr = NULL;
	int size;
	char salign[32];
	char *scol;
	int scaned;
	int i, rows, cnt;
	char buf[2048];
	FONT_SIZE_DEF *fdef;
	SIZE leading;
	TEXT t;
	RECT r;
	typedef struct
	{
		TEXT_ALIGN align;
		COORD basex;
	} col_format_t;
	col_format_t colfmt[MAX_TEXT_COLS];

	if (*istr < 0 || *istr >= GetStringTableCount (CreditsTab))
	{	// check if next one is within range
		int next_s = *istr + dir;

		if (next_s < 0 || next_s >= GetStringTableCount (CreditsTab))
			return 0;

		*istr = next_s;
	}

	// skip empty lines
	while (*istr >= 0 && *istr < GetStringTableCount (CreditsTab))
	{
		pStr = GetStringAddress (
				SetAbsStringTableIndex (CreditsTab, *istr));
		*istr += dir;
		if (pStr && *pStr != '\0')
			break;
	}

	if (!pStr || *pStr == '\0')
		return 0;
	
	if (2 != sscanf (pStr, "%d %31s %n", &size, salign, &scaned)
			|| size <= 0)
		return 0;
	pStr += scaned;
	
	utf8StringCopy (buf, sizeof (buf), pStr);
	rows = ParseTextLines (TextLines, MAX_TEXT_LINES, buf);
	if (rows == 0)
		return 0;
	// parse text columns
	for (i = 0, cnt = rows; i < rows; ++i)
	{
		char *nextcol;
		int icol;

		// we abuse the baseline here, but only a tiny bit
		// every line starts at col 0
		TextLines[i].baseline.x = 0;
		TextLines[i].baseline.y = i + 1;

		for (icol = 1, nextcol = strchr (TextLines[i].pStr, '\t');
				icol < MAX_TEXT_COLS && nextcol;
				++icol, nextcol = strchr (nextcol, '\t'))
		{
			*nextcol = '\0';
			++nextcol;

			if (cnt < MAX_TEXT_LINES)
			{
				TextLines[cnt].pStr = nextcol;
				TextLines[cnt].CharCount = ~0;
				TextLines[cnt].baseline.x = icol;
				TextLines[cnt].baseline.y = i + 1;
				++cnt;
			}
		}
	}

	// init alignments
	for (i = 0; i < MAX_TEXT_COLS; ++i)
	{
		colfmt[i].align = ALIGN_LEFT;
		colfmt[i].basex = SCREEN_WIDTH / 64;
	}

	// find the right font
	for (fdef = CreditsFont; fdef->size && size > fdef->size; ++fdef)
		;
	if (!fdef->size)
		return 0;

	t.align = ALIGN_LEFT;
	t.baseline.x = 100; // any value will do
	t.baseline.y = 100; // any value will do
	t.pStr = " ";
	t.CharCount = 1;

	LockMutex (GraphicsLock);
	OldContext = SetContext (TempContext);

	// get font dimensions
	SetContextFont (fdef->font);
	GetContextFontLeading (&leading);
	// get left/right margin
	TextRect (&t, &r, NULL_PTR);

	// parse text column alignment
	for (i = 0, scol = strtok (salign, ",");
			scol && i < MAX_TEXT_COLS;
			++i, scol = strtok (NULL, ","))
	{
		char c;
		int x;
		int n;

		// default
		colfmt[i].align = ALIGN_LEFT;
		colfmt[i].basex = r.extent.width;

		n = sscanf (scol, "%c/%d", &c, &x);
		if (n < 1)
		{	// DOES NOT COMPUTE! :)
			continue;
		}

		switch (c)
		{
			case 'L':
				colfmt[i].align = ALIGN_LEFT;
				if (n >= 2)
					colfmt[i].basex = x;
				break;
			case 'C':
				colfmt[i].align = ALIGN_CENTER;
				if (n >= 2)
					colfmt[i].basex = x;
				else
					colfmt[i].basex = SCREEN_WIDTH / 2;
				break;
			case 'R':
				colfmt[i].align = ALIGN_RIGHT;
				if (n >= 2)
					colfmt[i].basex = x;
				else
					colfmt[i].basex = SCREEN_WIDTH - r.extent.width;
				break;
		}
	}

	for (i = 0; i < cnt; ++i)
	{	
		// baseline contains coords in row/col quantities
		col_format_t *fmt = colfmt + TextLines[i].baseline.x;
		
		TextLines[i].align = fmt->align;
		TextLines[i].baseline.x = fmt->basex;
		TextLines[i].baseline.y *= leading;
	}

	f = Credits_MakeTextFrame (SCREEN_WIDTH, leading * rows + (leading >> 1),
			BackColor);
	OldFrame = SetContextFGFrame (f);
	// draw text
	SetContextForeGroundColor (ForeColor);
	for (i = 0; i < cnt; ++i)
		font_DrawText (TextLines + i);
	
	SetContextFGFrame (OldFrame);
	SetContext (OldContext);
	UnlockMutex (GraphicsLock);

	return f;
}

#define CREDIT_FRAMES 32

static int
credit_roll_task (void *data)
{
	Task task = (Task) data;
	CONTEXT OldContext;
	DWORD TimeIn;
	CONTEXT DrawContext;
	CONTEXT LocalContext;
	FRAME Frame;
	COLOR TransColor, TextFore, TextBack;
	STAMP TaskStamp;
	STAMP s;
	FRAME cf[CREDIT_FRAMES]; // preped text frames
	int ci[CREDIT_FRAMES];   // strtab indices
	int first = 0, last;
	int dy = 0, total_h, disp_h, deficit_h = 0;
	int strcnt;

	CreditsRunning = TRUE;

	memset(cf, 0, sizeof(cf));

	strcnt = GetStringTableCount (CreditsTab);

	TransColor = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x01);
	TextBack = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00);
	TextFore = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);

	LocalContext = CaptureContext (CreateContext ());
	DrawContext = CaptureContext (CreateContext ());
	
	total_h = disp_h = SCREEN_HEIGHT;

	LockMutex (GraphicsLock);
	// prep our local context
	OldContext = SetContext (LocalContext);
	
	// prep the first frame
	cf[0] = Credits_MakeTextFrame (1, total_h, TransColor);
	ci[0] = -1;
	last = 1;
	// prep the local screen copy
	Frame = Credits_MakeTextFrame (SCREEN_WIDTH, disp_h, TransColor);
	SetContextFGFrame (Frame);
	
	// prep our task context
	SetContext (DrawContext);
	SetContextFGFrame (Screen);

	SetContext (OldContext);
	UnlockMutex (GraphicsLock);

	TaskStamp.origin.x = TaskStamp.origin.y = 0;
	TaskStamp.frame = Frame;

	// stary background is the first screen frame
	LockMutex (GraphicsLock);
	OldContext = SetContext (LocalContext);
	// draw 
	s.origin.x = s.origin.y = 0;
	s.frame = CreditsBack;
	DrawStamp (&s);
	// draw clipped rect, if any
	if (OutTakesRunning)
	{
		SetContextForeGroundColor (TransColor);
		DrawFilledRectangle (&CommWndRect);
	}
	SetContext (OldContext);
	UnlockMutex (GraphicsLock);

	while (!Task_ReadState (task, TASK_EXIT))
	{
		RECT fr;
		int i;
		int rate, direction, dirstep;

		TimeIn = GetTimeCounter ();

		rate = abs (CreditsRate);
		if (rate != 0)
		{
			// scroll direction; forward or backward
			direction = CreditsRate / rate;
			// step in pixels
			dirstep = (rate + CREDITS_FRAME_RATE - 1) / CREDITS_FRAME_RATE;
			rate = ONE_SECOND * dirstep / rate;
			// step is also directional
			dirstep *= direction;
		}
		else
		{	// scroll stopped
			direction = 0;
			dirstep = 0;
			// one second interframe
			rate = ONE_SECOND;
		}

		// draw the credits
		//  comm animations play with contexts so we need to make
		//  sure the context is not desynced
		LockMutex (GraphicsLock);
		OldContext = SetContext (DrawContext);
		DrawStamp (&TaskStamp);
		SetContext (OldContext);
		FlushGraphics ();
		UnlockMutex (GraphicsLock);

		// prepare next screen frame
		dy += dirstep;
		// cap scroll
		if (dy < -(disp_h / 20))
		{	// at the begining
			// accelerated slow down
			if (CreditsRate < 0)
				CreditsRate -= CreditsRate / 10 - 1;
		}
		else if (deficit_h > disp_h / 25)
		{	// frame deficit -- task almost over
			// accelerated slow down
			if (CreditsRate > 0)
				CreditsRate -= CreditsRate / 10 + 1;

			CreditsRunning = (CreditsRate != 0);
		}
		else if (!CreditsRunning)
		{	// resumed
			CreditsRunning = TRUE;
		}

		if (first != last)
		{	// clean up frames that scrolled off the screen
			if (direction > 0)
			{	// forward scroll
				GetFrameRect (cf[first], &fr);
				if (dy >= (int)fr.extent.height)
				{	// past this frame already
					total_h -= fr.extent.height;
					DestroyDrawable (ReleaseDrawable (cf[first]));
					cf[first] = 0;
					// next frame
					first = (first + 1) % CREDIT_FRAMES;
					dy = 0;
				}
			}
			else if (direction < 0)
			{	// backward scroll
				int index = (last - 1 + CREDIT_FRAMES) % CREDIT_FRAMES; 
				
				GetFrameRect (cf[index], &fr);
				if (total_h - dy - (int)fr.extent.height >= disp_h)
				{	// past this frame already
					last = index;
					total_h -= fr.extent.height;
					DestroyDrawable (ReleaseDrawable (cf[last]));
					cf[last] = 0;
				}
			}
		}

		// render new text frames if needed
		if (direction > 0)
		{	// forward scroll
			int next_s = 0;

			// get next string
			if (first != last)
				next_s = ci[(last - 1 + CREDIT_FRAMES) % CREDIT_FRAMES] + 1;

			while (total_h - dy < disp_h && next_s < strcnt)
			{
				cf[last] = Credits_RenderTextFrame (LocalContext,
						&next_s, direction, TextBack, TextFore);
				ci[last] = next_s - 1;
				if (cf[last])
				{
					GetFrameRect (cf[last], &fr);
					total_h += fr.extent.height;
					
					last = (last + 1) % CREDIT_FRAMES;
				}
			}
		}
		else if (direction < 0)
		{	// backward scroll
			int next_s = strcnt - 1;

			// get next string
			if (first != last)
				next_s = ci[first] - 1;
			
			while (dy < 0 && next_s >= 0)
			{
				int index = (first - 1 + CREDIT_FRAMES) % CREDIT_FRAMES;
				cf[index] = Credits_RenderTextFrame (LocalContext,
						&next_s, direction, TextBack, TextFore);
				ci[index] = next_s + 1;
				if (cf[index])
				{
					GetFrameRect (cf[index], &fr);
					total_h += fr.extent.height;
					
					first = index;
					dy += fr.extent.height;
				}
			}
		}

		// draw next screen frame
		LockMutex (GraphicsLock);
		OldContext = SetContext (LocalContext);
		// draw background
		s.origin.x = s.origin.y = 0;
		s.frame = CreditsBack;
		DrawStamp (&s);
		// draw text frames
		s.origin.y = -dy;
		for (i = first; i != last; i = (i + 1) % CREDIT_FRAMES)
		{
			s.frame = cf[i];
			DrawStamp (&s);
			GetFrameRect (s.frame, &fr);
			s.origin.y += fr.extent.height;
		}
		if (direction > 0)
			deficit_h = disp_h - s.origin.y;

		// draw clipped rect, if any
		if (OutTakesRunning)
		{
			SetContextForeGroundColor (TransColor);
			DrawFilledRectangle (&CommWndRect);
		}
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);

		// wait till begining of next screen frame
		while (GetTimeCounter () < TimeIn + rate
				&& !Task_ReadState (task, TASK_EXIT))
		{
			SleepThreadUntil (ONE_SECOND / CREDITS_FRAME_RATE / 2);
		}
	}

	DestroyContext (ReleaseContext (DrawContext));
	DestroyContext (ReleaseContext (LocalContext));

	// free remaining frames
	DestroyDrawable (ReleaseDrawable (Frame));
	for ( ; first != last; first = (first + 1) % CREDIT_FRAMES)
		DestroyDrawable (ReleaseDrawable (cf[first]));

	FinishTask (task);
	return 0;
}

static Task
StartCredits (void)
{
	Task task;
	FONT_SIZE_DEF *fdef;

	CreditsTab = CaptureStringTable (LoadStringTable (CREDITS_STRTAB));
	if (!CreditsTab)
		return 0;
	CreditsBack = CaptureDrawable (LoadGraphic (CREDITS_BACK_ANIM));
	// load fonts
	for (fdef = CreditsFont; fdef->size; ++fdef)
		fdef->font = CaptureFont (LoadFont (fdef->res));

	CreditsRate = CREDITS_BASE_RATE;
	CreditsRunning = FALSE;

	task = AssignTask (credit_roll_task, 4096, "credit roll");

	return task;
}

static void
FreeCredits (void)
{
	FONT_SIZE_DEF *fdef;

	DestroyStringTable (ReleaseStringTable (CreditsTab));
	CreditsTab = 0;
	
	DestroyDrawable (ReleaseDrawable (CreditsBack));
	CreditsBack = 0;

	// free fonts
	for (fdef = CreditsFont; fdef->size; ++fdef)
	{
		DestroyFont (ReleaseFont (fdef->font));
		fdef->font = 0;
	}
}

static void
OutTakes (void)
{
#define NUM_OUTTAKES 15
	static long outtake_list[NUM_OUTTAKES] =
	{
		ZOQFOTPIK_CONVERSATION,
		TALKING_PET_CONVERSATION,
		ORZ_CONVERSATION,
		UTWIG_CONVERSATION,
		THRADD_CONVERSATION,
		SUPOX_CONVERSATION,
		SYREEN_CONVERSATION,
		SHOFIXTI_CONVERSATION,
		PKUNK_CONVERSATION,
		YEHAT_CONVERSATION,
		DRUUGE_CONVERSATION,
		URQUAN_CONVERSATION,
		VUX_CONVERSATION,
		BLACKURQ_CONVERSATION,
		ARILOU_CONVERSATION
	};

	BOOLEAN oldsubtitles = optSubtitles;
	int i = 0;

	optSubtitles = TRUE;
	sliderDisabled = TRUE;
	oscillDisabled = TRUE;

	for (i = 0; (i < NUM_OUTTAKES) &&
			!(GLOBAL (CurrentActivity) & CHECK_ABORT); i++)
	{
		InitCommunication (outtake_list[i]);
	}

	optSubtitles = oldsubtitles;
	sliderDisabled = FALSE;
	oscillDisabled = FALSE;
}

typedef struct
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (PVOID pInputState);
	COUNT MenuRepeatDelay;

	BOOLEAN AllowCancel;
	BOOLEAN AllowSpeedChange;
	BOOLEAN CloseWhenDone;
	DWORD CloseTimeOut;

} CREDITS_INPUT_STATE;

static BOOLEAN
DoCreditsInput (PVOID pIS)
{
	CREDITS_INPUT_STATE *pCIS = (CREDITS_INPUT_STATE *) pIS;

	if (CreditsRunning)
	{	// cancel timeout if resumed (or just running)
		pCIS->CloseTimeOut = 0;
	}

	if ((GLOBAL (CurrentActivity) & CHECK_ABORT)
			|| (pCIS->AllowCancel &&
				(PulsedInputState.menu[KEY_MENU_SELECT] ||
				PulsedInputState.menu[KEY_MENU_CANCEL]))
		)
	{	// aborted
		return FALSE;
	}
	
	if (pCIS->AllowSpeedChange
			&& (PulsedInputState.menu[KEY_MENU_UP]
			|| PulsedInputState.menu[KEY_MENU_DOWN]))
	{	// speed adjustment
		int newrate = CreditsRate;
		int step = abs (CreditsRate) / 5 + 1;

		if (PulsedInputState.menu[KEY_MENU_DOWN])
			newrate += step;
		else if (PulsedInputState.menu[KEY_MENU_UP])
			newrate -= step;
		if (newrate < -CREDITS_MAX_RATE)
			newrate = -CREDITS_MAX_RATE;
		else if (newrate > CREDITS_MAX_RATE)
			newrate = CREDITS_MAX_RATE;
			
		CreditsRate = newrate;
	}

	if (!CreditsRunning)
	{	// always allow cancelling once credits run through
		pCIS->AllowCancel = TRUE;
	}

	if (!CreditsRunning && pCIS->CloseWhenDone)
	{	// auto-close controlled by timeout
		if (pCIS->CloseTimeOut == 0)
		{	// set timeout
			pCIS->CloseTimeOut = GetTimeCounter () + CREDITS_TIMEOUT;
		}
		else if (GetTimeCounter () > pCIS->CloseTimeOut)
		{	// all done!
			return FALSE;
		}
	}
	
	if (!CreditsRunning
			&& (PulsedInputState.menu[KEY_MENU_SELECT]
			|| PulsedInputState.menu[KEY_MENU_CANCEL]))
	{	// credits finished and exit requested
		return FALSE;
	}

	SleepThread (ONE_SECOND / 30);

	return TRUE;
}

void
Credits (BOOLEAN WithOuttakes)
{
	BYTE fade_buf[1];
	MUSIC_REF hMusic;
	Task CredTask;
	CREDITS_INPUT_STATE cis;

	hMusic = LoadMusic (CREDITS_MUSIC);

	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	SetContextClipRect (NULL_PTR);
	SetContextBackGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));
	ClearDrawable ();
	fade_buf[0] = FadeAllToColor;
	XFormColorMap ((COLORMAPPTR)fade_buf, 0);
	UnlockMutex (GraphicsLock);

	// set the position of outtakes comm
	CommWndRect.corner.x = ((SCREEN_WIDTH - SIS_SCREEN_WIDTH) >> 1);
	CommWndRect.corner.y = 5;
	
	// launch credits and wait for the task to start up
	CredTask = StartCredits ();
	while (CredTask && !CreditsRunning
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		UpdateInputState ();
		SleepThread(ONE_SECOND / 20);
	}
	
	if (WithOuttakes)
	{
		OutTakesRunning = TRUE;
		OutTakes ();
		OutTakesRunning = FALSE;
	}
	
	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		if (hMusic)
			PlayMusic (hMusic, TRUE, 1);

		// nothing to do now but wait until credits
		//  are done or canceled by user
		cis.MenuRepeatDelay = 0;
		cis.InputFunc = DoCreditsInput;
		cis.AllowCancel = !WithOuttakes;
		cis.CloseWhenDone = !WithOuttakes;
		cis.AllowSpeedChange = TRUE;
		SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		DoInput (&cis, TRUE);
	}

	if (CredTask)
		ConcludeTask (CredTask);

	FadeMusic (0, ONE_SECOND / 2);
	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	fade_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));
	FlushColorXForms ();
	UnlockMutex (GraphicsLock);

	if (hMusic)
	{
		StopMusic ();
		DestroyMusic (hMusic);
	}
	FadeMusic (NORMAL_VOLUME, 0);

	FreeCredits ();
}
