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


/****************************************

  Star Control II: 3DO => SDL Port

  Copyright Toys for Bob, 2002

  Programmer: Chris Nelson
  
*****************************************/

#if defined (GFXMODULE_SDL_OPENGL) || defined (GFXMODULE_SDL_PURE)

#include "sdl_common.h"

#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>


//Status: Implemented!
DWORD
GetTimeCounter () //I wonder if it's ms, ticks, or seconds...
{
	DWORD ret;

	ret = (DWORD) (SDL_GetTicks () * ONE_SECOND / 1000.0);
	return (ret);
}

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


// following stuff was in getbody.c before modularization

extern char *_cur_resfile_name;

static void
process_image (FRAMEPTR FramePtr, SDL_Surface *img[], int cel_ct)
{
	int hx, hy, h;

	TYPE_SET (FramePtr->TypeIndexAndFlags, WANT_PIXMAP << FTYPE_SHIFT);
	INDEX_SET (FramePtr->TypeIndexAndFlags, cel_ct);

	hx = hy = 0;

	SDL_LockSurface (img[cel_ct]);
	if (img[cel_ct]->w >= 3 && (h = img[cel_ct]->h))
	{
		if (img[cel_ct]->format->palette)
		{
			int w;
			BYTE *pixel, *p, Cknockout, Chotx, Choty;

			img[cel_ct]->clip_rect.x = img[cel_ct]->w;
			img[cel_ct]->clip_rect.y = img[cel_ct]->h;
			img[cel_ct]->clip_rect.w = img[cel_ct]->clip_rect.h = 0;
			p = pixel = img[cel_ct]->pixels;
			Cknockout = *p++;
			Chotx = *p;
			*p++ = Cknockout;
			Choty = *p;
			*p++ = Cknockout;
			SDL_SetColorKey
			(
				img[cel_ct], SDL_SRCCOLORKEY,
				SDL_MapRGB
				(
					img[cel_ct]->format,
					img[cel_ct]->format->palette->colors[Cknockout].r,
					img[cel_ct]->format->palette->colors[Cknockout].g,
					img[cel_ct]->format->palette->colors[Cknockout].b
				)
			);
			do
			{
				w = img[cel_ct]->w - (p - pixel);
				if (w)
				{
					int x, y;
					BYTE opval;

					opval = Cknockout;
					do
					{
						BYTE pval;

						pval = *p;
						if (pval == Chotx)
						{
							*p = Cknockout;
							hx = img[cel_ct]->w - w;
						}
						if (pval == Choty)
						{
							*p = Cknockout;
							hy = img[cel_ct]->h - h;
						}

						pval = *p;
						if (pval == Cknockout)
						{
							if (opval != Cknockout)
							{
								x = img[cel_ct]->w - w;
								if (x > img[cel_ct]->clip_rect.x + img[cel_ct]->clip_rect.w)
									img[cel_ct]->clip_rect.w = x - img[cel_ct]->clip_rect.x;
								y = img[cel_ct]->h - h;
								if (y >= img[cel_ct]->clip_rect.y + img[cel_ct]->clip_rect.h)
									img[cel_ct]->clip_rect.h = y - img[cel_ct]->clip_rect.y + 1;
							}
						}
						else
						{
							if (opval == Cknockout)
							{
								x = img[cel_ct]->w - w;
								if (x < img[cel_ct]->clip_rect.x)
								{
									if (img[cel_ct]->clip_rect.w == 0)
										img[cel_ct]->clip_rect.w = 1;
									else
										img[cel_ct]->clip_rect.w += img[cel_ct]->clip_rect.x - x;
									img[cel_ct]->clip_rect.x = x;
								}
								y = img[cel_ct]->h - h;
								if (y < img[cel_ct]->clip_rect.y)
								{
									if (img[cel_ct]->clip_rect.h == 0)
										img[cel_ct]->clip_rect.h = 1;
									else
										img[cel_ct]->clip_rect.h += img[cel_ct]->clip_rect.y - y;
									img[cel_ct]->clip_rect.y = y;
								}
							}
						}
						opval = pval;
					} while (++p, --w);
					if (opval != Cknockout)
					{
						x = img[cel_ct]->w - w;
						if (x > img[cel_ct]->clip_rect.x + img[cel_ct]->clip_rect.w)
							img[cel_ct]->clip_rect.w = x - img[cel_ct]->clip_rect.x;
						y = img[cel_ct]->h - h;
						if (y >= img[cel_ct]->clip_rect.y + img[cel_ct]->clip_rect.h)
							img[cel_ct]->clip_rect.h = y - img[cel_ct]->clip_rect.y + 1;
					}
				}
				p = (pixel += img[cel_ct]->pitch);
			} while (--h);
		}
		else
		{
		}
	}
	SDL_UnlockSurface (img[cel_ct]);

hx -= img[cel_ct]->clip_rect.x;
hy -= img[cel_ct]->clip_rect.y;
	FramePtr->DataOffs = (BYTE *)TFB_LoadImage (img[cel_ct]) - (BYTE *)FramePtr;
img[cel_ct] = ((TFB_Image *)((BYTE *)FramePtr + FramePtr->DataOffs))->SurfaceSDL;
hx += img[cel_ct]->clip_rect.x;
hy += img[cel_ct]->clip_rect.y;
	SetFrameHotSpot (FramePtr, MAKE_HOT_SPOT (hx, hy));
	SetFrameBounds (FramePtr, img[cel_ct]->clip_rect.w, img[cel_ct]->clip_rect.h);
#if 0
printf ("\thot[%d, %d], rect[%d, %d, %d, %d]\n",
		hx, hy,
		img[cel_ct]->clip_rect.x,
		img[cel_ct]->clip_rect.y,
		img[cel_ct]->clip_rect.w,
		img[cel_ct]->clip_rect.h);
#endif
}

MEM_HANDLE
_GetCelData (FILE *fp, DWORD length)
{
#ifdef WIN32
	int omode;
#endif
	int cel_ct, n;
	DWORD opos;
	char CurrentLine[1024], filename[1024];
#define MAX_CELS 256
	SDL_Surface *img[MAX_CELS];
	DRAWABLE Drawable;
	
	opos = ftell (fp);

	{
		char *s1, *s2;

		if (_cur_resfile_name == 0
				|| (((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
						&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0))
			n = 0;
		else
		{
			if (s2 > s1)
				s1 = s2;
			n = s1 - _cur_resfile_name + 1;
			strncpy (filename, _cur_resfile_name, n);
		}
	}

#ifdef WIN32
	omode = _setmode (fileno (fp), O_TEXT);
#endif
	cel_ct = 0;
	while (fgets (CurrentLine, sizeof (CurrentLine), fp) && cel_ct < MAX_CELS)
	{
		if
		(
				sscanf(CurrentLine, "%s", &filename[n]) == 1
				&& (img[cel_ct] = IMG_Load (filename))
				&& img[cel_ct]->w > 0
				&& img[cel_ct]->h > 0
				&& img[cel_ct]->format->BitsPerPixel >= 8
		)
			++cel_ct;
		else if (img[cel_ct])
printf("_GetCelData: Bad file!\n"),
			SDL_FreeSurface (img[cel_ct]);

		if ((int)ftell (fp) - (int)opos >= (int)length)
			break;
	}
#ifdef WIN32
	_setmode (fileno (fp), omode);
#endif

	Drawable = 0;
	if (cel_ct && (Drawable = AllocDrawable (cel_ct, 0)))
	{
		DRAWABLEPTR DrawablePtr;

		if ((DrawablePtr = LockDrawable (Drawable)) == 0)
		{
			while (cel_ct--)
				SDL_FreeSurface (img[cel_ct]);

			mem_release (Drawable);
			Drawable = 0;
		}
		else
		{
			FRAMEPTR FramePtr;

			DrawablePtr->hDrawable = GetDrawableHandle (Drawable);
			TYPE_SET (DrawablePtr->FlagsAndIndex,
					(DRAWABLE_TYPE)ROM_DRAWABLE << FTYPE_SHIFT);
			INDEX_SET (DrawablePtr->FlagsAndIndex, cel_ct - 1);

			FramePtr = &DrawablePtr->Frame[cel_ct];
			while (--FramePtr, cel_ct--)
				process_image (FramePtr, img, cel_ct);

			UnlockDrawable (Drawable);
		}
	}

if (Drawable == 0)
	printf ("Couldn't get cel data for '%s'\n", _cur_resfile_name);
	return (GetDrawableHandle (Drawable));
}

BOOLEAN
_ReleaseCelData (MEM_HANDLE handle)
{
	DRAWABLEPTR DrawablePtr;

	if ((DrawablePtr = LockDrawable (handle)) == 0)
		return (FALSE);

	if (TYPE_GET (DrawablePtr->FlagsAndIndex) != SCREEN_DRAWABLE)
	{
		int cel_ct;
		FRAMEPTR FramePtr;

		cel_ct = INDEX_GET (DrawablePtr->FlagsAndIndex);

		FramePtr = &DrawablePtr->Frame[cel_ct];
		while (--FramePtr, cel_ct--)
		{
			if (FramePtr->DataOffs)
			{
				TFB_Image *img;

				img = (TFB_Image *)((BYTE *)FramePtr + FramePtr->DataOffs);
				FramePtr->DataOffs = 0;

				TFB_FreeImage (img);
			}
		}
	}

	UnlockDrawable (handle);
	mem_release (handle);

	return (TRUE);
}

MEM_HANDLE
_GetFontData (FILE *fp, DWORD length)
{
	DWORD cel_ct;
	COUNT num_entries;
	FONT_REF FontRef;
	DIRENTRY FontDir;
	BOOLEAN found_chars;
#define MAX_CELS 256
	SDL_Surface *img[MAX_CELS] = { 0 };
	char pattern[1024];
	
	if (_cur_resfile_name == 0)
		return (0);
	sprintf (pattern, "%s/*.*", _cur_resfile_name);

	found_chars = FALSE;
	FontDir = CaptureDirEntryTable (LoadDirEntryTable (pattern, &num_entries));
	while (num_entries--)
	{
		char *char_name;

		char_name = GetDirEntryAddress (SetAbsDirEntryTableIndex (FontDir, num_entries));
		if (sscanf (char_name, "%u.", (unsigned int) &cel_ct) == 1
				&& cel_ct >= FIRST_CHAR
				&& img[cel_ct -= FIRST_CHAR] == 0
				&& sprintf (pattern, "%s/%s", _cur_resfile_name, char_name)
				&& (img[cel_ct] = IMG_Load (pattern)))
		{
			if (img[cel_ct]->w > 0
					&& img[cel_ct]->h > 0
					&& img[cel_ct]->format->BitsPerPixel >= 8)
				found_chars = TRUE;
			else
			{
				SDL_FreeSurface (img[cel_ct]);
				img[cel_ct] = 0;
			}
		}
		
	}
	DestroyDirEntryTable (ReleaseDirEntryTable (FontDir));

	FontRef = 0;
	if (found_chars && (FontRef = AllocFont (0)))
	{
		FONTPTR FontPtr;

		cel_ct = MAX_CELS; // MAX_CHARS;
		if ((FontPtr = LockFont (FontRef)) == 0)
		{
			while (cel_ct--)
			{
				if (img[cel_ct])
					SDL_FreeSurface (img[cel_ct]);
			}

			mem_release (FontRef);
			FontRef = 0;
		}
		else
		{
			FRAMEPTR FramePtr;

			FontPtr->FontRef = FontRef;
			FontPtr->Leading = 0;
			FramePtr = &FontPtr->CharDesc[cel_ct];
			while (--FramePtr, cel_ct--)
			{
				if (img[cel_ct])
				{
					process_image (FramePtr, img, cel_ct);
					SetFrameBounds (FramePtr, GetFrameWidth (FramePtr) + 1, GetFrameHeight (FramePtr));
					if (img[0])
						SetFrameHotSpot (FramePtr, MAKE_HOT_SPOT (0, img[0]->clip_rect.h));
					if (GetFrameHeight (FramePtr) > FontPtr->Leading)
						FontPtr->Leading = GetFrameHeight (FramePtr);
				}
			}
			++FontPtr->Leading;

			UnlockFont (FontRef);
		}
	}
	(void) fp;  /* Satisfying compiler (unused parameter) */
	(void) length;  /* Satisfying compiler (unused parameter) */
	return (FontRef);
}

BOOLEAN
_ReleaseFontData (MEM_HANDLE handle)
{
	FONTPTR FontPtr;

	if ((FontPtr = LockFont (handle)) == 0)
		return (FALSE);

	{
		int cel_ct;
		FRAMEPTR FramePtr;

		cel_ct = MAX_CHARS;

		FramePtr = &FontPtr->CharDesc[cel_ct];
		while (--FramePtr, cel_ct--)
		{
			if (FramePtr->DataOffs)
			{
				TFB_Image *img;

				img = (TFB_Image *)((BYTE *)FramePtr + FramePtr->DataOffs);
				FramePtr->DataOffs = 0;

				TFB_FreeImage (img);
			}
		}
	}

	UnlockFont (handle);
	mem_release (handle);

	return (TRUE);
}

// _request_drawable was in drawable.c before modularization

DRAWABLE
_request_drawable (COUNT NumFrames, DRAWABLE_TYPE DrawableType,
		CREATE_FLAGS flags, SIZE width, SIZE height)
{
	DRAWABLE Drawable;

	Drawable = AllocDrawableImage (
			NumFrames, DrawableType, flags, width, height
			);
	if (Drawable)
	{
		DRAWABLEPTR DrawablePtr;

		if ((DrawablePtr = LockDrawable (Drawable)) == 0)
		{
			FreeDrawable (Drawable);
			Drawable = 0;
		}
		else
		{
			int imgw, imgh;
			FRAMEPTR FramePtr;

			DrawablePtr->hDrawable = GetDrawableHandle (Drawable);
			TYPE_SET (DrawablePtr->FlagsAndIndex, flags << FTYPE_SHIFT);
			INDEX_SET (DrawablePtr->FlagsAndIndex, NumFrames - 1);

			imgw = (flags & MAPPED_TO_DISPLAY) ? width * ScreenWidthActual / ScreenWidth : width;
			imgh = (flags & MAPPED_TO_DISPLAY) ? height * ScreenHeightActual / ScreenHeight : height;
			FramePtr = &DrawablePtr->Frame[NumFrames - 1];
			while (NumFrames--)
			{
				TFB_Image *Image;


				if (DrawableType == RAM_DRAWABLE
						&& (Image = TFB_LoadImage (SDL_CreateRGBSurface (
										SDL_HWSURFACE,
										imgw,
										imgh,
										24,
										0x00FF0000,
										0x0000FF00,
										0x000000FF,
										0x00000000
										))))
					FramePtr->DataOffs = (BYTE *)Image - (BYTE *)FramePtr;

				TYPE_SET (FramePtr->TypeIndexAndFlags, DrawableType);
				INDEX_SET (FramePtr->TypeIndexAndFlags, NumFrames);
				SetFrameBounds (FramePtr, width, height);
				--FramePtr;
			}
			UnlockDrawable (Drawable);
		}
	}

	return (Drawable);
}

#endif
