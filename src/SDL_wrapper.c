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

/******************************************

  Star Control II: 3DO => SDL Port

  Copyright Toys for Bob, 2002

  3DO_wrapper.cpp programmer: Chris Nelson

*******************************************/

//

#include "libs/graphics/gfxintrn.h"
#include "libs/input/inpintrn.h"
#include "SDL_wrapper.h"

//#if SDL_BYTEORDER == SDL_BIG_ENDIAN

#define NUMBER_OF_PLUTVALS 32
#define NUMBER_OF_PLUT_UINT32s (NUMBER_OF_PLUTVALS >> 1)
#define GET_VAR_PLUT(i) (_varPLUTs + (i) * NUMBER_OF_PLUT_UINT32s)
DWORD* _varPLUTs; //Not a function
PDISPLAY_INTERFACE _pCurDisplay; //Not a function. Probably has to be initialized...

void (*mask_func_array[])
		(PRECT pClipRect, PRIMITIVEPTR PrimPtr)
		= { 0 };

//Status: Implemented!
DWORD
GetTimeCounter () //I wonder if it's ms, ticks, or seconds...
{
	DWORD ret;

	ret = (DWORD) (SDL_GetTicks () * ONE_SECOND / 1000.0);
	return (ret);
}

void
BatchGraphics (void)
{
}

void
UnbatchGraphics (void)
{
}

//Status: Unimplemented
DWORD
XFormColorMap (COLORMAPPTR colormap, SIZE time)
		// Not sure what the time's supposed to mean...
{
		// printf ("Unimplemented function activated: XFormColorMap()\n");
		return (GetTimeCounter () + time);
}

static volatile int continuity_break;

void
FlushGraphics (void)
{
	continuity_break = 1;
}

void
DrawFromExtraScreen (PRECT r) //No scaling?
{
	RECT locRect;
	TFB_DrawCommand* DC;

	if (!r)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = SCREEN_WIDTH;
		locRect.extent.height = SCREEN_HEIGHT;
		r = &locRect;
	}

	DC=HMalloc(sizeof(TFB_DrawCommand));

	DC->Type = TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER;
	DC->x = r->corner.x;
	DC->y = r->corner.y;
	DC->w = r->extent.width;
	DC->h = r->extent.height;

	TFB_EnqueueDrawCommand (DC);
}

void
SetGraphicUseOtherExtra (int other) //Could this possibly be more cryptic?!? :)
{
}

void
LoadIntoExtraScreen (PRECT r) //like a glTexture?
{
	RECT locRect;
	TFB_DrawCommand *DC;

	if (!r)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = SCREEN_WIDTH;
		locRect.extent.height = SCREEN_HEIGHT;
		r = &locRect;
	}

// printf ("LoadIntoExtraScreen (%d, %d, {%d, %d})\n", r->corner.x, r->corner.y, r->extent.width, r->extent.height);
	DC = HMalloc (sizeof (TFB_DrawCommand));

	DC->Type = TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER;
	DC->x = r->corner.x;
	DC->y = r->corner.y;
	DC->w = r->extent.width;
	DC->h = r->extent.height;
	DC->image = 0;

	TFB_EnqueueDrawCommand (DC);
}

void
SetGraphicGrabOther (int grab_other)
{
}

//Status: Unimplemented
void
ScreenTransition (int TransType, PRECT pRect) //TransType==3
{
		//printf ("Unimplemented function activated: ScreenTransition()\n");
}

//Status: Unimplemented
UNICODE
KBDToUNICODE (UNICODE SK_in)
		//This one'll probably require a big table. Arg.
{
	return (SK_in);
}

//Status: Unimplemented
BOOLEAN
SetColorMap (COLORMAPPTR map)
		//There's also a macro for thing in appglue.h... Hmm...
{
		BOOLEAN ret;

		//printf("Unimplemented function activated: SetColorMap()\n");
		ret = TRUE;
		return (ret);
}

//Status: Unimplemented
void
StopSound()
		// So does this stop ALL sound? How about music?
{
	Mix_HaltChannel (-1);
}

//Status: Ignored
BOOLEAN
UninitInput () // Looks like it'll be empty
{
		BOOLEAN ret;

		// printf("Unimplemented function activated: UninitInput()\n");
		ret = TRUE;
		return (ret);
}

static DRAWABLE
alloc_image (COUNT NumFrames, DRAWABLE_TYPE DrawableType, CREATE_FLAGS
		flags, SIZE width, SIZE height)
{
	DWORD data_byte_count;
	DRAWABLE Drawable;

	data_byte_count = 0;
	if (flags & WANT_MASK)
		data_byte_count += (DWORD) SCAN_WIDTH (width) * height;
	if ((flags & WANT_PIXMAP) && DrawableType == RAM_DRAWABLE)
	{
		width = ((width << 1) + 3) & ~3;
		data_byte_count += (DWORD) width * height;
	}

	Drawable = AllocDrawable (NumFrames, data_byte_count * NumFrames);
	if (Drawable)
	{
		if (DrawableType == RAM_DRAWABLE)
		{
			COUNT i;
			DWORD data_offs;
			DRAWABLEPTR DrawablePtr;
			FRAMEPTR F;

			data_offs = sizeof (*F) * NumFrames;
			DrawablePtr = LockDrawable (Drawable);
			for (i = 0, F = &DrawablePtr->Frame[0]; i < NumFrames; ++i, ++F)
			{
				F->DataOffs = data_offs;
				data_offs += data_byte_count - sizeof (*F);
			}
			UnlockDrawable (Drawable);
		}
	}

	return (Drawable);
}

static int gscale;

static void
blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	TFB_Image *img;
	PFRAME_DESC SrcFramePtr;

	SrcFramePtr = (PFRAME_DESC)PrimPtr->Object.Stamp.frame;
	if (SrcFramePtr->DataOffs == 0)
	{
		printf ("Non-existent image to blt()\n");
		return;
	}

	img = (TFB_Image *) ((BYTE *) SrcFramePtr + SrcFramePtr->DataOffs);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawCommand *DrawCommand;

		DrawCommand = HMalloc (sizeof (TFB_DrawCommand));
		if (DrawCommand)
		{
			DrawCommand->Type = TFB_DRAWCOMMANDTYPE_IMAGE;
			DrawCommand->x = pClipRect->corner.x -
					GetFrameHotX (_CurFramePtr);
			DrawCommand->y = pClipRect->corner.y -
					GetFrameHotY (_CurFramePtr);
			DrawCommand->w = img->SurfaceSDL->clip_rect.w;
			DrawCommand->h = img->SurfaceSDL->clip_rect.h;
			if (gscale)
			{
				DrawCommand->x += (GetFrameHotX (SrcFramePtr) *
						((1 << 8) - gscale)) >> 8;
				DrawCommand->y += (GetFrameHotY (SrcFramePtr) *
						((1 << 8) - gscale)) >> 8;
				DrawCommand->w = (DrawCommand->w * gscale) >> 8;
				DrawCommand->h = (DrawCommand->h * gscale) >> 8;
			}
			DrawCommand->image = img; //TFB_Image
			if (GetPrimType (PrimPtr) == STAMPFILL_PRIM)
			{
				DWORD c32k;

				c32k = GetPrimColor (PrimPtr) >> 8;  // shift out color index
				DrawCommand->r = (c32k >> (10 - (8 - 5))) & 0xF8;
				DrawCommand->g = (c32k >> (5 - (8 - 5))) & 0xF8;
				DrawCommand->b = (c32k << (8 - 5));
			}

			DrawCommand->Qualifier = (TYPE_GET (GetFrameParentDrawable (
					SrcFramePtr)->FlagsAndIndex) >> FTYPE_SHIFT) &
					MAPPED_TO_DISPLAY;

			TFB_EnqueueDrawCommand(DrawCommand);
		}
	}
	else
	{
		SDL_Rect SDL_SrcRect, SDL_DstRect;
		
		SDL_SrcRect.x = (short) img->SurfaceSDL->clip_rect.x;
		SDL_SrcRect.y = (short) img->SurfaceSDL->clip_rect.y;
		SDL_SrcRect.w = (short) img->SurfaceSDL->clip_rect.w;
		SDL_SrcRect.h = (short) img->SurfaceSDL->clip_rect.h;

		SDL_DstRect.x = (short) pClipRect->corner.x -
						GetFrameHotX (_CurFramePtr) + SDL_SrcRect.x;
		SDL_DstRect.y = (short) pClipRect->corner.y -
						GetFrameHotY (_CurFramePtr) + SDL_SrcRect.y;

		SDL_BlitSurface (
			img->SurfaceSDL,  // src
			&SDL_SrcRect,     // src rect
			((TFB_Image *) ((BYTE *) _CurFramePtr +
			_CurFramePtr->DataOffs))->SurfaceSDL,
			&SDL_DstRect      // dst rect
		);
	}
}

static void
fillrect_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	BYTE r, g, b;
	DWORD c32k;

	c32k = GetPrimColor (PrimPtr) >> 8; //shift out color index
	r = (c32k >> (10 - (8 - 5))) & 0xF8;
	g = (c32k >> (5 - (8 - 5))) & 0xF8;
	b = (c32k << (8 - 5));

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawCommand *DrawCommand;

		DrawCommand = HMalloc (sizeof (TFB_DrawCommand));
		if (DrawCommand)
		{
			DrawCommand->Type = TFB_DRAWCOMMANDTYPE_RECTANGLE;
			DrawCommand->x = pClipRect->corner.x - GetFrameHotX (_CurFramePtr);
			DrawCommand->y = pClipRect->corner.y - GetFrameHotY (_CurFramePtr);
			DrawCommand->w = pClipRect->extent.width;
			DrawCommand->h = pClipRect->extent.height;
			DrawCommand->r = r;
			DrawCommand->g = g;
			DrawCommand->b = b;

			if (gscale && GetPrimType (PrimPtr) != POINT_PRIM)
			{
				DrawCommand->w = (DrawCommand->w * gscale) >> 8;
				DrawCommand->h = (DrawCommand->h * gscale) >> 8;
				DrawCommand->x += (pClipRect->extent.width -
						DrawCommand->w) >> 1;
				DrawCommand->y += (pClipRect->extent.height -
						DrawCommand->h) >> 1;
			}

			TFB_EnqueueDrawCommand(DrawCommand);
		}
	}
	else
	{
		SDL_Rect  SDLRect;
		SDL_Surface *surf;

		surf = ((TFB_Image *) ((BYTE *) _CurFramePtr +
				_CurFramePtr->DataOffs))->SurfaceSDL;

		SDLRect.x = (short) (pClipRect->corner.x -
				GetFrameHotX (_CurFramePtr));
		SDLRect.y = (short) (pClipRect->corner.y -
				GetFrameHotY (_CurFramePtr));
		SDLRect.w = (short) pClipRect->extent.width;
		SDLRect.h = (short) pClipRect->extent.height;

		SDL_FillRect (surf, &SDLRect, SDL_MapRGB (surf->format, r, g, b));
	}
}

static void
cmap_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
// printf ("Unimplemented function activated: cmap_blt()\n");
}

static void
line_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	float x1,y1,x2,y2;
	BYTE r, g, b;
	DWORD c32k;

	c32k = GetPrimColor (PrimPtr) >> 8;  // shift out color index
	r = (c32k >> (10 - (8 - 5))) & 0xF8;
	g = (c32k >> (5 - (8 - 5))) & 0xF8;
	b = (c32k << (8 - 5));

	x1=PrimPtr->Object.Line.first.x - GetFrameHotX (_CurFramePtr);
	y1=PrimPtr->Object.Line.first.y - GetFrameHotY (_CurFramePtr);
	x2=PrimPtr->Object.Line.second.x - GetFrameHotX (_CurFramePtr);
	y2=PrimPtr->Object.Line.second.y - GetFrameHotY (_CurFramePtr);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawCommand *DC;

		DC = HMalloc (sizeof (TFB_DrawCommand));

		DC->Type = TFB_DRAWCOMMANDTYPE_LINE;
		DC->x = x1;
		DC->y = y1;
		DC->w = x2;
		DC->h = y2;
		DC->r = r;
		DC->g = g;
		DC->b = b;

		TFB_EnqueueDrawCommand(DC);
	}
	else
	{
	}
}

static void
read_screen (PRECT lpRect, FRAMEPTR DstFramePtr)
{
	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) != SCREEN_DRAWABLE
			|| TYPE_GET (DstFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE
			|| !(TYPE_GET (GetFrameParentDrawable (DstFramePtr)
			->FlagsAndIndex)
			& ((DWORD) MAPPED_TO_DISPLAY << FTYPE_SHIFT)))
	{
		printf("Unimplemented function activated: read_screen()\n");
	}
	else
	{
		TFB_DrawCommand* DC;

		DC = HMalloc(sizeof(TFB_DrawCommand));

		DC->Type = TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER;
		DC->x = lpRect->corner.x;
		DC->y = lpRect->corner.y;
		DC->w = lpRect->extent.width;
		DC->h = lpRect->extent.height;
		DC->image = (TFB_Image *) ((BYTE *) DstFramePtr +
				DstFramePtr->DataOffs);

		TFB_EnqueueDrawCommand (DC);
	}
}

void (*func_array[]) (PRECT pClipRect, PRIMITIVEPTR PrimPtr) =
{
	fillrect_blt,
	blt,
	blt,
	line_blt,
	_text_blt,
	cmap_blt,
	_rect_blt,
	fillrect_blt,
	_fillpoly_blt,
	_fillpoly_blt,
};

static DISPLAY_INTERFACE DisplayInterface =
{
	WANT_MASK,

	16, // SCREEN_DEPTH,
	320,
	240,

	alloc_image,
	read_screen,

	func_array,
};

void
LoadDisplay (PDISPLAY_INTERFACE *pDisplay)
{
	*pDisplay = &DisplayInterface;
}

static TFB_DrawCommandQueue *DrawCommandQueue;
static void *ExtraScreen;

//Status: Not entirely unimplemented!
BOOLEAN
InitGraphics (int argc, char* argv[], COUNT KbytesRequired)
		// Kbytes should only matter if we wanted to port Starcon2 to the
		// hand-helds...
{
	BOOLEAN ret;

	LoadDisplay (&_pCurDisplay);
	mem_init ();
	ActivateDisplay ();

	DrawCommandQueue = TFB_DrawCommandQueue_Create ();

	ExtraScreen = HMalloc (ScreenWidthActual * ScreenHeightActual * 4);
	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
void
UninitGraphics ()  // Also probably empty
{
	HFree (DrawCommandQueue);

	HFree (ExtraScreen);
	mem_uninit ();
}

// Status: Unimplemented
void
UninitSound ()  //Maybe I should call StopSound() first?
{
	//printf("Unimplemented function activated: UninitSound()\n");
}

// Status: Ignored
void
UninitVideo ()  //empty
{
	// printf("Unimplemented function activated: UninitVideo()\n");
}

static int _music_volume = (MAX_VOLUME + 1) >> 1;

// Status: Unimplemented
void
SetMusicVolume(COUNT Volume) //Volume calibration... [0,255]?
{
	Mix_VolumeMusic ((_music_volume = Volume) * SDL_MIX_MAXVOLUME /
			MAX_VOLUME);
}

static TASK FadeTask;
static SIZE TTotal;
static SIZE volume_end;

static int
fade_task (void *data)
{
	SIZE TDelta, volume_beg;
	DWORD StartTime, CurTime;

	volume_beg = _music_volume;
	StartTime = CurTime = GetTimeCounter ();
	do
	{
		CurTime = SleepTask (CurTime + 1);
		if ((TDelta = (SIZE) (CurTime - StartTime)) > TTotal)
			TDelta = TTotal;

		SetMusicVolume ((COUNT) (volume_beg + (SIZE)
				((long) (volume_end - volume_beg) * TDelta / TTotal)));
	} while (TDelta < TTotal);

	FadeTask = 0;
	return (1);
}

DWORD
FadeMusic (BYTE end_vol, SIZE TimeInterval)
{
	DWORD TimeOut;

	if (FadeTask)
	{
		volume_end = _music_volume;
		TTotal = 1;
		do
			TaskSwitch ();
		while (FadeTask);
		TaskSwitch ();
	}

	if ((TTotal = TimeInterval) <= 0)
		TTotal = 1; /* prevent divide by zero and negative fade */
	volume_end = end_vol;
		
	if (TTotal > 1 && (FadeTask = AddTask (fade_task, 0)))
	{
		TimeOut = GetTimeCounter () + TTotal + 1;
	}
	else
	{
		SetMusicVolume (end_vol);
		TimeOut = GetTimeCounter ();
	}

	return (TimeOut);
}

//Status: Unimplemented
void
FlushColorXForms() //Not entirely sure what's done here. CLUT stuff?
{
// printf ("Unimplemented function activated: FlushColorXForms()\n");
}

//Status: Unimplemented
BOOLEAN
SoundPlaying()
{
	return (Mix_Playing (-1) != 0);
}

//Status: Unimplemented
void
VidStop() // At least this one makes sense.
{
		// printf ("Unimplemented function activated: VidStop()\n");
}

//Status: Unimplemented
MEM_HANDLE
VidPlaying()  // Easy enough.
{
		MEM_HANDLE ret;

		//printf("Unimplemented function activated: VidPlaying()\n");
		ret = 0;
		return (ret);
}

//Status: Unimplemented
VIDEO_TYPE
VidPlay (VIDEO_REF VidRef, char *loopname, BOOLEAN uninit)
		// uninit? when done, I guess.
{
	VIDEO_TYPE ret;
	ret = 0;

// printf("Unimplemented function activated: VidPlay()\n");
	return (ret);
}

int
GetSoundData(char* data) //Returns the data size.
{
	int ret;

	printf("Unimplemented function activated: GetSoundData()\n");
	ret = 0;
	return (ret);
}

// Status: Unimplemented
int
GetSoundInfo(int* length, int* offset)
		// Umm... How does it know which sound?
{
	int ret;

	printf ("Unimplemented function activated: GetSoundInfo()\n");
	ret = 0;
	return (ret);
}

// Status: Unimplemented
BOOLEAN
ChannelPlaying(COUNT WhichChannel)
		// SDL will have a nice interface for this, I hope.
{
	// printf("Unimplemented function activated: ChannelPlaying()\n");
	return ((BOOLEAN) Mix_Playing (WhichChannel));
}

// Status: Unimplemented
void
SetGraphicScale (int scale)
		//Calibration...
{
	gscale = scale;
}

// Status: Unimplemented
void
PlayChannel(
		COUNT channel,
		PVOID sample,
		COUNT sample_length,
		COUNT loop_begin,
		COUNT loop_length,
		unsigned char priority
)  // This one's important.
{
	Mix_Chunk* Chunk;

	Chunk=*(Mix_Chunk**)sample;
	Mix_PlayChannel (
		channel, //-1 would be easiest
		Chunk,
		0
	);
}

//Status: Unimplemented
void
StopChannel(COUNT channel, unsigned char Priority)  // Easy w/ SDL.
{
	Mix_HaltChannel (channel);
}

// Status: Maybe Implemented
PBYTE
GetSampleAddress (SOUND sound)
		// I might be prototyping this wrong, type-wise.
{
	return ((PBYTE)GetSoundAddress (sound));
}

// Status: Unimplemented
COUNT
GetSampleLength (SOUND sound)
{
	COUNT ret;
	Mix_Chunk *Chunk;
		
	Chunk = (Mix_Chunk *) sound;
	ret = 0;  // Chunk->alen;

// printf ("Maybe Implemented function activated: GetSampleLength()\n");
	return(ret);
}

// Status: Unimplemented
void
SetChannelRate (COUNT channel, DWORD rate_hz, unsigned char priority)
		// in hz
{
// printf ("Unimplemented function activated: SetChannelRate()\n");
}

// Status: Unimplemented
COUNT
GetSampleRate (SOUND sound)
{
	COUNT ret;

// printf("Unimplemented function activated: GetSampleRate()\n");
	ret=0;
	return(ret);
}

// Status: Unimplemented
void
SetChannelVolume (COUNT channel, COUNT volume, BYTE priority)
		// I wonder what this whole priority business is...
		// I can probably ignore it.
{
// printf ("Unimplemented function activated: SetChannelVolume()\n");
}

static UNICODE PauseKey;
static UNICODE ExitKey;
static UNICODE OtherKey;

static BOOLEAN (* PauseFunc) (void);

// Status: Unimplemented
extern BOOLEAN
InitInput (UNICODE Pause, UNICODE Exit, BOOLEAN (*PFunc) (void))
//Be careful with this one.
{
	PauseKey = Pause;
	ExitKey = Exit;
		
	// Find a unique byte ID not equal to PauseKey, ExitKey, or 0.
	OtherKey = 1;
	while (OtherKey == PauseKey || OtherKey == ExitKey)
		OtherKey++;
				
	PauseFunc = PFunc;

	return (TRUE);
}

INPUT_STATE
AnyButtonPress (BOOLEAN DetectSpecial)
{
	int i;

	if (DetectSpecial)
	{
		if (KeyDown (PauseKey) && PauseFunc && (*PauseFunc) ())
			;
	}

	for (i = 0; i < sizeof (KeyboardDown) / sizeof (KeyboardDown[0]); ++i)
	{
		if (KeyboardDown[i])
			return (DEVICE_BUTTON1);
	}
#if 0
	for (i = 0; i < JOYSTICKS_AVAIL; i++)
		if (read_joystick (i))
			return (DEVICE_BUTTON1);
#endif
		
	return (0);
}

// Status: Ignored
BOOLEAN
InitVideo (BOOLEAN useCDROM) //useCDROM doesn't really apply to us...
{
	BOOLEAN ret;

// printf("Unimplemented function activated: InitVideo()\n");
	ret = TRUE;
	return (ret);
}

//Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
		// Odd things to pass the InitSound function...
{
	BOOLEAN ret;

// printf("Unimplemented function activated: InitSound()\n");
	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
void
SetGraphicStrength (int numerator, int denominator)
		// I just hope numerator always = denominator...
{
// printf("Unimplemented function activated: SetGrahicsStrength()\n");
}

// Status: Unimplemented
UNICODE
KeyHit () // Does this clear the top of a queue, or just read it?
{
	int i;

	for (i = 0; i < sizeof (KeyboardDown) / sizeof (KeyboardDown[0]); ++i)
	{
		if (KeyboardDown[i])
		{
			if (i == SDLK_RETURN)
				return ('\n');
			else if (i >= 256)
			{
				switch (i)
				{
					case SDLK_KP_ENTER:
						return ('\n');
					case SDLK_KP4:
					case SDLK_LEFT:
						return (SK_LF_ARROW);
					case SDLK_KP6:
					case SDLK_RIGHT:
						return (SK_RT_ARROW);
					case SDLK_KP8:
					case SDLK_UP:
						return (SK_UP_ARROW);
					case SDLK_KP2:
					case SDLK_DOWN:
						return (SK_DN_ARROW);
					case SDLK_KP7:
					case SDLK_HOME:
						return (SK_HOME);
					case SDLK_KP9:
					case SDLK_PAGEUP:
						return (SK_PAGE_UP);
					case SDLK_KP1:
					case SDLK_END:
						return (SK_END);
					case SDLK_KP3:
					case SDLK_PAGEDOWN:
						return (SK_PAGE_DOWN);
					case SDLK_KP0:
					case SDLK_INSERT:
						return (SK_INSERT);
					case SDLK_KP_PERIOD:
					case SDLK_DELETE:
						return (SK_DELETE);
					case SDLK_KP_PLUS:
						return (SK_KEYPAD_PLUS);
					case SDLK_KP_MINUS:
						return (SK_KEYPAD_MINUS);
					case SDLK_LSHIFT:
						return (SK_LF_SHIFT);
					case SDLK_RSHIFT:
						return (SK_RT_SHIFT);
					case SDLK_LCTRL:
					case SDLK_RCTRL:
						return (SK_CTL);
					case SDLK_LALT:
					case SDLK_RALT:
						return (SK_ALT);
					case SDLK_F1:
					case SDLK_F2:
					case SDLK_F3:
					case SDLK_F4:
					case SDLK_F5:
					case SDLK_F6:
					case SDLK_F7:
					case SDLK_F8:
					case SDLK_F9:
					case SDLK_F10:
					case SDLK_F11:
					case SDLK_F12:
						return ((UNICODE) ((i - SDLK_F1) + SK_F1));
				}
				continue;
			}

			return ((UNICODE) i);
		}
	}

	return (0);
}

//Status: Unimplemented
int
KeyDown (UNICODE which_scan)
		// This might use SK_* stuff, of just plain old chars
{
	int i;

	i = which_scan;
	if (i == '\n')
	{
		if (KeyboardDown[SDLK_KP_ENTER])
			return (1);
		i = SDLK_RETURN;
	}
	else if (i >= 128)
	{
		switch (i)
		{
			case SK_LF_ARROW:
				if (KeyboardDown[SDLK_KP4])
					return (1);
				i = SDLK_LEFT;
				break;
			case SK_RT_ARROW:
				if (KeyboardDown[SDLK_KP6])
					return (1);
				i = SDLK_RIGHT;
				break;
			case SK_UP_ARROW:
				if (KeyboardDown[SDLK_KP8])
					return (1);
				i = SDLK_UP;
				break;
			case SK_DN_ARROW:
				if (KeyboardDown[SDLK_KP2])
					return (1);
				i = SDLK_DOWN;
				break;
			case SK_HOME:
				if (KeyboardDown[SDLK_KP7])
					return (1);
				i = SDLK_HOME;
				break;
			case SK_PAGE_UP:
				if (KeyboardDown[SDLK_KP9])
					return (1);
				i = SDLK_PAGEUP;
				break;
			case SK_END:
				if (KeyboardDown[SDLK_KP1])
					return (1);
				i = SDLK_END;
				break;
			case SK_PAGE_DOWN:
				if (KeyboardDown[SDLK_KP3])
					return (1);
				i = SDLK_PAGEDOWN;
				break;
			case SK_INSERT:
				if (KeyboardDown[SDLK_KP0])
					return (1);
				i = SDLK_INSERT;
				break;
			case SK_DELETE:
				if (KeyboardDown[SDLK_KP_PERIOD])
					return (1);
				i = SDLK_DELETE;
				break;
			case SK_KEYPAD_PLUS:
				i = SDLK_KP_PLUS;
				break;
			case SK_KEYPAD_MINUS:
				i = SDLK_KP_MINUS;
				break;
			case SK_LF_SHIFT:
				i = SDLK_LSHIFT;
				break;
			case SK_RT_SHIFT:
				i = SDLK_RSHIFT;
				break;
			case SK_CTL:
				if (KeyboardDown[SDLK_LCTRL])
					return (1);
				i = SDLK_RCTRL;
				break;
			case SK_ALT:
				if (KeyboardDown[SDLK_LALT])
					return (1);
				i = SDLK_RALT;
				break;
			case SK_F1:
			case SK_F2:
			case SK_F3:
			case SK_F4:
			case SK_F5:
			case SK_F6:
			case SK_F7:
			case SK_F8:
			case SK_F9:
			case SK_F10:
			case SK_F11:
			case SK_F12:
				i = (i - SK_F1) + SDLK_F1;
		}
	}

	return (KeyboardDown[i]);
}

// Status: Unimplemented
BOOLEAN
BatchColorMap (COLORMAPPTR ColorMapPtr)
		// Batch... I wonder what that verb means in this context?
		// SvdB: I don't think it is a verb.
{
	BOOLEAN ret;

// printf("Unimplemented function activated: BatchColorMap()\n");
	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
BOOLEAN
_image_intersect (PIMAGE_BOX box1, PIMAGE_BOX box2, PRECT rect)
		// This is a biggie.
{
	BOOLEAN ret;

// printf ("Unimplemented function activated: _image_intersect()\n");
	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
INPUT_STATE
_get_serial_keyboard_state (INPUT_REF ref, INPUT_STATE InputState)
		// I hope these are defined somewhere...
{
	Uint16 key;
	extern Uint16 GetUNICODEKey (void);

	if ((key = GetUNICODEKey ()) == '\r')
		key = '\n';
	SetInputUNICODE (&InputState, key);
	SetInputDevType (&InputState, KEYBOARD_DEVICE);

	return (InputState);
}

// Status: Unimplemented
INPUT_STATE
_get_joystick_keyboard_state (INPUT_REF InputRef, INPUT_STATE InputState)
		// see line above.
{
	SBYTE dx, dy;
	BYTEPTR KeyEquivalentPtr;

	KeyEquivalentPtr = GetInputDeviceKeyEquivalentPtr (InputRef);
	dx = (SBYTE)(KeyDown (KeyEquivalentPtr[1]) -
			KeyDown (KeyEquivalentPtr[0]));
	SetInputXComponent (&InputState, dx);

	KeyEquivalentPtr += 2;
	dy = (SBYTE)(KeyDown (KeyEquivalentPtr[1]) -
			KeyDown (KeyEquivalentPtr[0]));
	SetInputYComponent (&InputState, dy);

	KeyEquivalentPtr += 2;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON1;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON2;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON3;

	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_LEFTSHIFT;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_RIGHTSHIFT;

	if (InputState)
		SetInputDevType (&InputState, JOYSTICK_DEVICE);

	return (InputState);
}

/*
// Status: Implemented
INPUT_STATE
_get_joystick_state (INPUT_REF ref, INPUT_STATE InputState)
		// consistant, at least.
{
// printf ("Half-implemented function activated: _get_joystick_state()\n");

	if (KeyboardStroke[SDLK_LEFT])
	{
		SetInputXComponent (&InputState, -1);
// printf ("LEFT!\n");
		KeyboardStroke[SDLK_LEFT] = FALSE;
	}
	if (KeyboardStroke[SDLK_RIGHT])
	{
		SetInputXComponent (&InputState, 1);
// printf ("RIGHT!\n");
		KeyboardStroke[SDLK_RIGHT] = FALSE;
	}

	if (KeyboardStroke[SDLK_UP])
	{
		SetInputYComponent (&InputState, -1);
// printf ("UP!\n");
		KeyboardStroke[SDLK_UP] = FALSE;
	}
	if (KeyboardStroke[SDLK_DOWN])
	{
		SetInputYComponent (&InputState, 1);
// printf ("DOWN!\n");
		KeyboardStroke[SDLK_DOWN] = FALSE;
	}

	if (KeyboardStroke[ControlA])
	{
		InputState |= DEVICE_BUTTON1;
// printf("BUTTON1!\n");
		KeyboardStroke[ControlA]=FALSE;
	}
	if (KeyboardStroke[ControlB])
	{
		InputState |= DEVICE_BUTTON2;
// printf ("BUTTON2!\n");
		KeyboardStroke[ControlB] = FALSE;
	}
	if (KeyboardStroke[ControlC])
	{
		InputState |= DEVICE_BUTTON3;
// printf ("BUTTON3!\n");
		KeyboardStroke[ControlC] = FALSE;
	}
	if (KeyboardStroke[ControlX])
	{
		InputState |= DEVICE_EXIT;
// printf ("BUTTONX!\n");
		KeyboardStroke[ControlX] = FALSE;
	}
	if (KeyboardStroke[ControlStart])
	{
		InputState |= DEVICE_PAUSE;
// printf ("CONTROLSTART!\n");
		KeyboardStroke[ControlStart] = FALSE;
	}
	if (KeyboardStroke[ControlLeftShift])
	{
		InputState |= DEVICE_LEFTSHIFT;
// printf ("LEFTSHIFT!\n");
		KeyboardStroke[ControlLeftShift] = FALSE;
	}
	if (KeyboardStroke[ControlRightShift])
	{
		InputState |= DEVICE_RIGHTSHIFT;
// printf ("RIGHTSHIFT!\n");
		KeyboardStroke[ControlRightShift] = FALSE;
	}
	
	return (InputState);
}
*/

INPUT_STATE
_get_joystick_state (INPUT_REF ref, INPUT_STATE InputState)
		// consistant, at least.
{
#if 0
	if (ref == JoystickInput[1] && 0)
	{
		//printf ("ref\n");
		
		return(0);
	}
	
// printf ("Half-implemented function activated: _get_joystick_state()\n");

	if (KeyboardDown[SDLK_LEFT])
	{
		SetInputXComponent (&InputState, -1);
		//printf ("LEFT!\n");
	}
	if (KeyboardDown[SDLK_RIGHT])
	{
		SetInputXComponent (&InputState, 1);
		//printf ("RIGHT!\n");
	}

	if (KeyboardDown[SDLK_UP])
	{
		SetInputYComponent (&InputState, -1);
		//printf ("UP!\n");
	}
	if (KeyboardDown[SDLK_DOWN])
	{
		SetInputYComponent (&InputState, 1);
		//printf ("DOWN!\n");
	}

	if (KeyboardDown[ControlA])
	{
		InputState |= DEVICE_BUTTON1;
		//printf ("BUTTON1!\n");
	}
	if (KeyboardDown[ControlB])
	{
		InputState |= DEVICE_BUTTON2;
		//printf ("BUTTON2!\n");
	}
	if (KeyboardDown[ControlC])
	{
		InputState |= DEVICE_BUTTON3;
		//printf ("BUTTON3!\n");
	}
	if (KeyboardDown[ControlX])
	{
		InputState |= DEVICE_EXIT;
		//printf ("BUTTONX!\n");
	}
	if (KeyboardDown[ControlStart])
	{
		InputState |= DEVICE_PAUSE;
		//printf ("CONTROLSTART!\n");
	}
	if (KeyboardDown[ControlLeftShift])
	{
		InputState |= DEVICE_LEFTSHIFT;
		//printf ("LEFTSHIFT!\n");
	}
	if (KeyboardDown[ControlRightShift])
	{
		InputState |= DEVICE_RIGHTSHIFT;
		//printf ("RIGHTSHIFT!\n");
	}
#endif
	
	return (InputState);
}

// Status: Ignored - The keyboard will serve as the joystick, thus a port is always open.
BOOLEAN
_joystick_port_active(COUNT port) //SDL handles this nicely.
{
	printf ("Unimplemented function activated: _joystick_port_active()\n");
	return (0);
}

INPUT_STATE
_get_pause_exit_state (void)
{
	INPUT_STATE InputState;

	InputState = 0;
	if (KeyDown (PauseKey))
	{
		if (PauseFunc && (*PauseFunc) ())
			;
	}
	else if (KeyDown (ExitKey))
	{
		InputState = DEVICE_EXIT;
	}
#if 0
	for (i = 0; i < JOYSTICKS_AVAIL; i++)
	{
		DWORD joy;
		
		joy = read_joystick (i);
		if (joy & ControlStart) // pause
		{
			if (PauseFunc && (*PauseFunc) ())
// while (KeyDown (PauseKey))
				;
		}
		else if (joy & ControlX) // exit
		{
// while (KeyDown (ExitKey))
// TaskSwitch ();

			InputState = DEVICE_EXIT;
		}
	}
#endif

	return (InputState);
}

DWORD
SleepTask (DWORD wake_time)
{
	DWORD t;

	t = GetTimeCounter ();
	if (wake_time <= t)
		SDL_Delay (0);
	else
		SDL_Delay ((wake_time - t) * 1000 / ONE_SECOND);

	return (GetTimeCounter ());
}

VIDEO_REF
_init_video_file(PVOID pStr)
{
	VIDEO_REF ret;

	printf ("Unimplemented function activated: _init_video_file()\n");
	ret = 0;
	return (ret);
}

// Draw Command Queue Stuff
TFB_DrawCommandQueue*
TFB_DrawCommandQueue_Create()
{
		TFB_DrawCommandQueue* myQueue;
		
		myQueue = (TFB_DrawCommandQueue*) HMalloc(
				sizeof(TFB_DrawCommandQueue));

		myQueue->Back = 0;
		myQueue->Front = 0;
		myQueue->Size = 0;

		return (myQueue);
}

void
TFB_DrawCommandQueue_Push (TFB_DrawCommandQueue* myQueue,
		TFB_DrawCommand* Command)
{
	TFB_DrawCommandNode* NewNode;

	NewNode = (TFB_DrawCommandNode*) HMalloc (sizeof (TFB_DrawCommandNode));
	NewNode->Command = Command;
	NewNode->Behind = NewNode;

	if (myQueue->Size == 0)
	{
		NewNode->Ahead = NewNode;
		myQueue->Front = NewNode;
	}
	else
	{
		NewNode->Ahead = myQueue->Back;
		myQueue->Back->Behind = NewNode;
	}

	myQueue->Back=NewNode;
	myQueue->Size++;
}

TFB_DrawCommand *
TFB_DrawCommandQueue_Pop (TFB_DrawCommandQueue *myQueue)
{
	TFB_DrawCommand *ReturnMe;
	TFB_DrawCommandNode *NextFront;

	if (myQueue->Size == 0)
		return (0);

	ReturnMe = myQueue->Front->Command;
	NextFront = myQueue->Front->Behind;
	HFree (myQueue->Front);
	if (myQueue->Size == 1)
	{
		myQueue->Front=0;
		myQueue->Back=0;
	}
	else
	{
		myQueue->Front=NextFront;
		myQueue->Front->Ahead=myQueue->Front;
	}

	myQueue->Size--;

	return (ReturnMe);
}

void
TFB_DeallocateDrawCommand (TFB_DrawCommand* Command)
{
	HFree(Command);
}

//

DWORD
SetSemaphore (SEMAPHORE *sem)
{
	SDL_SemWait (*sem);
	return (GetTimeCounter ());
}

void
ClearSemaphore (SEMAPHORE *sem)
{
	SDL_SemPost (*sem);
}

TASK
AddTask (TASK_FUNC arg1, COUNT arg2)
{
	return (SDL_CreateThread ((int (*)(void *)) arg1, NULL));
}

void
DeleteTask (TASK T)
{
	SDL_KillThread (T);
}

static int Next2Power (int in)
{
	int c;
	for(c = 1; c <= in ; c *= 2)
		;

	return(c);
}

TFB_Image* TFB_LoadImage (SDL_Surface *img)
{
	TFB_Image* myImage;

	myImage = (TFB_Image*) HMalloc (sizeof (TFB_Image));

	// Create a new SDL Image

	if (img->format->BytesPerPixel < 4)
	{
		SDL_Surface *new_surf;
				
		new_surf = SDL_CreateRGBSurface (
				SDL_SWSURFACE,
				img->clip_rect.w,
				img->clip_rect.h,
				32,
				0x000000FF,
				0x0000FF00,
				0x00FF0000,
				0xFF000000
		);
		if (new_surf)
		{
			SDL_Rect SDL_DstRect;
					
			SDL_DstRect.x = SDL_DstRect.y = 0;
			/*
			SDL_FillRect (
					new_surf,
					&SDL_DstRect,
					SDL_MapRGBA(new_surf->format, 1, 1, 1, .5)
			);
			*/
			SDL_BlitSurface (
					img,              // src
					&img->clip_rect,  // src rect
					new_surf,         // dst
					&SDL_DstRect      // dst rect
			);

			new_surf->clip_rect.x = new_surf->clip_rect.y = 0;
			new_surf->clip_rect.w = img->clip_rect.w;
			new_surf->clip_rect.h = img->clip_rect.h;

			SDL_FreeSurface (img);
			img = new_surf;
		}
	}
	myImage->SurfaceSDL=img;

	// Figure out glTexture* Variables

	myImage->glTextureW = Next2Power (myImage->SurfaceSDL->clip_rect.w);
	myImage->glTextureH = Next2Power (myImage->SurfaceSDL->clip_rect.h);

	myImage->glTexture = 0;

	return(myImage);
}

void
TFB_FreeImage (TFB_Image *img)
{
	if (img)
	{
		TFB_DrawCommand* DC;

		DC = HMalloc (sizeof (TFB_DrawCommand));

		DC->Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC->image = img;

		TFB_EnqueueDrawCommand (DC);
	}
}

void
TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}
		
	if (DrawCommand->Type <= TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER
			&& TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		static RECT scissor_rect;

		// Set the clipping region.
		if (scissor_rect.corner.x != _pCurContext->ClipRect.corner.x
				|| scissor_rect.corner.y != _pCurContext->ClipRect.corner.y
				|| scissor_rect.extent.width !=
				_pCurContext->ClipRect.extent.width
				|| scissor_rect.extent.height !=
				_pCurContext->ClipRect.extent.height)
		{
			// Enqueue command to set the glScissor spec

			TFB_DrawCommand* DC;

			scissor_rect = _pCurContext->ClipRect;

			DC = HMalloc (sizeof (TFB_DrawCommand));

			DC->Type = scissor_rect.extent.width
					? (DC->x = scissor_rect.corner.x,
					DC->y=scissor_rect.corner.y,
					DC->w=scissor_rect.extent.width,
					DC->h=scissor_rect.extent.height),
					TFB_DRAWCOMMANDTYPE_SCISSORENABLE
					: TFB_DRAWCOMMANDTYPE_SCISSORDISABLE;

			TFB_EnqueueDrawCommand(DC);
		}
	}

	TFB_DrawCommandQueue_Push (DrawCommandQueue, DrawCommand);
}

void
TFB_FlushGraphics () //Only call from main thread!!
{
#define NUM_FLUSH_PASSES 1
	int semval, pass;
	TFB_DrawCommandQueue DCQ_Copy, DCQ_Swap;
	Uint32 this_flush;
	static Uint32 last_flush = 0;

	this_flush = SDL_GetTicks ();
	if (this_flush - last_flush < 1000 / 100)
		return;

	semval = SDL_SemTryWait (GraphicsSem);
	if (semval != 0 && !continuity_break)
		return;

	continuity_break = 0;
	if (DrawCommandQueue == 0 || DrawCommandQueue->Size == 0)
	{
		if (semval == 0)
			SDL_SemPost (GraphicsSem);
		return;
	}

	last_flush = this_flush;

	DCQ_Copy = *DrawCommandQueue;
	DrawCommandQueue->Back = 0;
	DrawCommandQueue->Front = 0;
	DrawCommandQueue->Size = 0;
	DCQ_Swap = *DrawCommandQueue;
	if (semval == 0)
		SDL_SemPost (GraphicsSem);

	glDrawBuffer (GL_BACK);
	for (pass = 0; pass < NUM_FLUSH_PASSES; ++pass)
	{
		while (DCQ_Copy.Size > 0)
		{
			TFB_DrawCommand* DC;

			DC = TFB_DrawCommandQueue_Pop (&DCQ_Copy);

			if (DC == 0)
			{
				printf ("Woah there coyboy! Trouble with TFB_Queues...\n");
				continue;
			}

			switch (DC->Type)
			{
				case TFB_DRAWCOMMANDTYPE_IMAGE:
					if (DC->image==0)
					{
						printf ("Error!!! Bad Image!\n");
					}
					else
					{
						int vx, vy;
						float y0, y1;
						
						glMatrixMode (GL_PROJECTION);
						glPushMatrix ();

						//

						glLoadIdentity ();
						if (DC->Qualifier != MAPPED_TO_DISPLAY)
						{
							glOrtho (0,ScreenWidth,ScreenHeight, 0, -1, 1);
							vx = DC->x;
							vy = DC->y;
							y0 = 0;
							y1 = DC->image->SurfaceSDL->clip_rect.h /
									DC->image->glTextureH;
						}
						else
						{
							glOrtho (0, ScreenWidthActual,
									ScreenHeightActual, 0, -1, 1);
							vx = DC->x * ScreenWidthActual / ScreenWidth;
							vy = DC->y * ScreenHeightActual / ScreenHeight;
							y0 = DC->image->SurfaceSDL->clip_rect.h /
									DC->image->glTextureH;
							y1 = 0;
						}
						glMatrixMode (GL_MODELVIEW);
						glLoadIdentity ();

						vx += DC->image->SurfaceSDL->clip_rect.x;
						vy += DC->image->SurfaceSDL->clip_rect.y;
						if (DC->image->glTexture)
						{
							glBindTexture (GL_TEXTURE_2D,DC->image->glTexture);
						}
						else
						{
							//Load Image into glTexture
							
							glGenTextures(1, &(DC->image->glTexture));
							glBindTexture(GL_TEXTURE_2D, DC->image->glTexture);
							glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
							glPixelStorei(GL_UNPACK_ROW_LENGTH, DC->image->SurfaceSDL->w);
							glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
// glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							glTexImage2D // Make a blank texture
							(
									GL_TEXTURE_2D,
									0,
									GL_RGBA,                // All incoming SDL_Surfaces must have an alpha channel
									DC->image->glTextureW,
									DC->image->glTextureH,
									0,
									GL_RGBA,                // All glTextures will have an alpha channel
									GL_UNSIGNED_BYTE,
									0                       // Valid in OpenGL 1.1 or later
							);
							SDL_LockSurface(DC->image->SurfaceSDL);
							glTexSubImage2D (
									GL_TEXTURE_2D, // Must be GL_TEXTURE_2D
									0, // Level of detail number (mipmapping)
									DC->image->SurfaceSDL->clip_rect.x,  // X
									DC->image->SurfaceSDL->clip_rect.y,  // Y
									DC->image->SurfaceSDL->clip_rect.w,  // Width
									DC->image->SurfaceSDL->clip_rect.h,  // Height
									GL_RGBA,                             // Format
									GL_UNSIGNED_BYTE,                    // Type
									DC->image->SurfaceSDL->pixels        // Pixels
							);
							SDL_UnlockSurface(DC->image->SurfaceSDL);
						}
						glEnable (GL_TEXTURE_2D);
						glEnable (GL_BLEND);
						glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
						glColor4f (1,1,1,1);
						glBegin (GL_QUADS);

						glTexCoord2f (0,y0);
						glVertex2i (vx, vy);

						glTexCoord2f (0,y1);
						glVertex2i (vx, vy + DC->h);

						glTexCoord2f (DC->image->SurfaceSDL->clip_rect.w / DC->image->glTextureW, y1);
						glVertex2i (vx + DC->w, vy + DC->h);

						glTexCoord2f (DC->image->SurfaceSDL->clip_rect.w / DC->image->glTextureW, y0);
						glVertex2i (vx + DC->w, vy);

						glEnd ();
						glDisable (GL_BLEND);
						glDisable (GL_TEXTURE_2D);

						//

						glPopMatrix ();
						glMatrixMode (GL_MODELVIEW);
					}
					break;
				case TFB_DRAWCOMMANDTYPE_LINE:
					glMatrixMode (GL_PROJECTION);
					glPushMatrix ();

					glLoadIdentity ();
					glOrtho (0, ScreenWidth, ScreenHeight, 0, -1, 1);
					glMatrixMode (GL_MODELVIEW);
					glLoadIdentity ();
					
					//
					
					glColor3f ((float) DC->r / 255.0, (float) DC->g / 255.0,
							(float) DC->b / 255.0);
					glBegin (GL_LINES);
					{
						glVertex2f (DC->x,DC->y);
						glVertex2f (DC->w,DC->h);
					}
					glEnd ();

					//

					glPopMatrix ();
					glMatrixMode (GL_MODELVIEW);
					break;
				case TFB_DRAWCOMMANDTYPE_RECTANGLE:
					glMatrixMode (GL_PROJECTION);
					glPushMatrix ();

					//

					glLoadIdentity ();
					glOrtho (0, ScreenWidth, ScreenHeight, 0, -1, 1);
					glMatrixMode (GL_MODELVIEW);
					glLoadIdentity ();

					glBegin (GL_QUADS);
					{
						glColor3f (((float) DC->r) / 255.0,
								((float) DC->g) / 255.0,
								((float) DC->b) / 255.0);
						glVertex2i (DC->x        , DC->y + DC->h);
						glVertex2i (DC->x        , DC->y        );
						glVertex2i (DC->x + DC->w, DC->y        );
						glVertex2i (DC->x + DC->w, DC->y + DC->h);
					}
					glEnd ();

					//

					glPopMatrix ();
					glMatrixMode (GL_MODELVIEW);
					break;
				case TFB_DRAWCOMMANDTYPE_SCISSORENABLE:
				{
					int x;
					int y;
					int w;
					int h;

					x = DC->x * ScreenWidthActual / ScreenWidth;
					y = (ScreenHeight - (DC->y + DC->h)) *
							ScreenHeightActual / ScreenHeight;
					w = DC->w * ScreenWidthActual / ScreenWidth;
					h = DC->h * ScreenHeightActual / ScreenHeight;
					
					glEnable (GL_SCISSOR_TEST);
					glScissor (x, y, w, h);
					break;
				}
				case TFB_DRAWCOMMANDTYPE_SCISSORDISABLE:
					glDisable (GL_SCISSOR_TEST);
					break;
				case TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER:
					if (pass == 0)
					{
						int x, y, w, h;
						void *pixels;

						glMatrixMode (GL_PROJECTION);
						glPushMatrix ();
						glLoadIdentity ();

						glOrtho(0, ScreenWidth,ScreenHeight, 0, -1, 1);
						glMatrixMode (GL_MODELVIEW);
						glLoadIdentity ();
						glRasterPos2d (0,0);

						x = DC->x * ScreenWidthActual / ScreenWidth;
						y = (ScreenHeight - (DC->y + DC->h)) *
								ScreenHeightActual / ScreenHeight;
						w = DC->w * ScreenWidthActual / ScreenWidth;
						h = DC->h * ScreenHeightActual / ScreenHeight;
						if (DC->image == 0)
						{
							glPixelStorei (GL_PACK_ROW_LENGTH,
									ScreenWidthActual);
							glPixelStorei (GL_PACK_SKIP_PIXELS, x);
							glPixelStorei (GL_PACK_SKIP_ROWS, y);
							pixels = ExtraScreen;
						}
						else
						{
							SDL_LockSurface (DC->image->SurfaceSDL);

							glPixelStorei (GL_PACK_ROW_LENGTH,
									DC->image->SurfaceSDL->w);
							glPixelStorei (GL_PACK_SKIP_PIXELS, 0);
							glPixelStorei (GL_PACK_SKIP_ROWS, 0);
							pixels = DC->image->SurfaceSDL->pixels;  // Pixels
						}

						glReadBuffer (GL_BACK);
// glFlush ();
//glFinish ();
						glReadPixels (x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
								pixels);
						glPixelStorei (GL_PACK_ROW_LENGTH, 0);

						if (DC->image)
						{
							SDL_UnlockSurface (DC->image->SurfaceSDL);
						}
						else
						{
							glPixelStorei (GL_PACK_SKIP_ROWS, 0);
							glPixelStorei (GL_PACK_SKIP_PIXELS, 0);
						}

						glPopMatrix ();
						glMatrixMode (GL_MODELVIEW);
					}
					break;
				case TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER:
				{
					int x, y, w, h;

					glMatrixMode (GL_PROJECTION);
					glPushMatrix ();
					glLoadIdentity ();
					glOrtho (0, ScreenWidth, ScreenHeight, 0, -1, 1);
					glMatrixMode (GL_MODELVIEW);
					glLoadIdentity ();
					glRasterPos2d (DC->x, DC->y + DC->h);

					x = DC->x * ScreenWidthActual / ScreenWidth;
					y = (ScreenHeight - (DC->y + DC->h)) *
							ScreenHeightActual / ScreenHeight;
					w = DC->w * ScreenWidthActual / ScreenWidth;
					h = DC->h * ScreenHeightActual / ScreenHeight;
					
					glPixelStorei (GL_UNPACK_ROW_LENGTH, ScreenWidthActual);
					glPixelStorei (GL_UNPACK_SKIP_PIXELS, x);
					glPixelStorei (GL_UNPACK_SKIP_ROWS, y);
					glDrawPixels (w, h, GL_RGBA, GL_UNSIGNED_BYTE,
							ExtraScreen);
					glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
					glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
					glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);

					glPopMatrix();
					glMatrixMode(GL_MODELVIEW);
					break;
				}
				case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
					if (pass == NUM_FLUSH_PASSES - 1)
					{
						SDL_FreeSurface (DC->image->SurfaceSDL);
						if (DC->image->glTexture)
							glDeleteTextures (1, &(DC->image->glTexture));
						HFree (DC->image);
					}
					break;
			}

			if (pass == NUM_FLUSH_PASSES - 1)
				TFB_DeallocateDrawCommand (DC);
			else
				TFB_DrawCommandQueue_Push (&DCQ_Swap, DC);
		}

		if (pass == 0)
		{
			GLboolean ScissorMode;

			glGetBooleanv (GL_SCISSOR_TEST, &ScissorMode);

			glFlush ();
			SDL_GL_SwapBuffers ();
// glFlush();
#if NUM_FLUSH_PASSES > 1
			DCQ_Copy = DCQ_Swap;
#else
			glReadBuffer (GL_FRONT);
			glDrawBuffer (GL_BACK);

			glMatrixMode (GL_PROJECTION);
			glPushMatrix ();
			glLoadIdentity ();

			glOrtho (0, ScreenWidthActual, ScreenHeightActual, 0, -1, 1);
			glMatrixMode (GL_MODELVIEW);
			glLoadIdentity ();
			glDisable (GL_SCISSOR_TEST);
			glRasterPos2d (0, ScreenHeightActual - .1);
					// Re "-.1": All values 0 < x < .5 work. Strange!!

			glCopyPixels (0, 0, ScreenWidthActual, ScreenHeightActual,
					GL_COLOR);

			glPopMatrix ();
			glMatrixMode (GL_MODELVIEW);

			if (ScissorMode)
			{
				glEnable (GL_SCISSOR_TEST);
			}
#endif
		}
	}
}
