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

#ifdef GFXMODULE_SDL

#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>

#include "sdl_common.h"
#include "primitives.h"

typedef struct anidata
{
	int transparent_color;
	int colormap_index;
	int hotspot_x;
	int hotspot_y;
} AniData;


extern char *_cur_resfile_name;

static void
process_image (FRAMEPTR FramePtr, SDL_Surface *img[], AniData *ani, int cel_ct)
{
	TFB_Image *tfbimg;
	int hx, hy;

	TYPE_SET (FramePtr->TypeIndexAndFlags, ROM_DRAWABLE);
	INDEX_SET (FramePtr->TypeIndexAndFlags, cel_ct);

	if (img[cel_ct]->format->palette)
	{
		if (ani[cel_ct].transparent_color != -1)
		    SDL_SetColorKey(img[cel_ct], SDL_SRCCOLORKEY, ani[cel_ct].transparent_color);
	}
	
	hx = ani[cel_ct].hotspot_x;
	hy = ani[cel_ct].hotspot_y;

	FramePtr->image = TFB_DrawImage_New (img[cel_ct]);

	tfbimg = FramePtr->image;
	tfbimg->colormap_index = ani[cel_ct].colormap_index;
	img[cel_ct] = (SDL_Surface *)tfbimg->NormalImg;

        FramePtr->HotSpot = MAKE_HOT_SPOT (hx, hy);
	SetFrameBounds (FramePtr, img[cel_ct]->w, img[cel_ct]->h);
}

static void
process_font (FRAMEPTR FramePtr, SDL_Surface *img[], int cel_ct)
{
	int hx, hy;
	int x,y;
	Uint8 r,g,b,a;
	Uint32 p;
	SDL_Color colors[256];
	SDL_Surface *new_surf;				
	GetPixelFn getpix;
	PutPixelFn putpix;

	TYPE_SET (FramePtr->TypeIndexAndFlags, WANT_PIXMAP << FTYPE_SHIFT);
	INDEX_SET (FramePtr->TypeIndexAndFlags, cel_ct);

	hx = hy = 0;

	SDL_LockSurface (img[cel_ct]);

	// convert 32-bit png font to indexed

	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, img[cel_ct]->w, img[cel_ct]->h, 
		8, 0, 0, 0, 0);

	getpix = getpixel_for (img[cel_ct]);
	putpix = putpixel_for (new_surf);

	for (y = 0; y < img[cel_ct]->h; ++y)
	{
		for (x = 0; x < img[cel_ct]->w; ++x)
		{
			p = getpix (img[cel_ct], x, y);
			SDL_GetRGBA (p, img[cel_ct]->format, &r, &g, &b, &a);

			if (r == 0 && g == 0 && b == 0)
				putpix (new_surf, x, y, 0);
			else
				putpix (new_surf, x, y, 1);
		}
	}

	colors[0].r = colors[0].g = colors[0].b = 0;
	for (x = 1; x < 256; ++x)
	{
		colors[x].r = 255;
		colors[x].g = 255;
		colors[x].b = 255;
	}

	SDL_SetColors (new_surf, colors, 0, 256);
	SDL_SetColorKey (new_surf, SDL_SRCCOLORKEY, 0);

	SDL_UnlockSurface (img[cel_ct]);
	SDL_FreeSurface (img[cel_ct]);

	img[cel_ct] = new_surf;
	
	FramePtr->image = TFB_DrawImage_New (img[cel_ct]);
	img[cel_ct] = FramePtr->image->NormalImg;
	
        FramePtr->HotSpot = MAKE_HOT_SPOT (hx, hy);
	SetFrameBounds (FramePtr, img[cel_ct]->w, img[cel_ct]->h);
}

//stretch_frame
// create a new frame of size neww x newh, and blit a scaled version FramePtr into it 
// destroy the old frame if 'destroy' is 1
FRAMEPTR stretch_frame (FRAMEPTR FramePtr, int neww, int newh, int destroy)
{
	FRAMEPTR NewFrame;
	UBYTE type;
	TFB_Image *tfbImg;
	TFB_Canvas src, dst;
	EXTENT ext;
	type = (UBYTE)TYPE_GET (GetFrameParentDrawable (FramePtr)
			->FlagsAndIndex) >> FTYPE_SHIFT;
	NewFrame = CaptureDrawable (
				CreateDrawable (type, (SIZE)neww, (SIZE)newh, 1)
					);
	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	src = tfbImg->NormalImg;
	dst = NewFrame->image->NormalImg;
	ext.width = neww;
	ext.height = newh;
	TFB_DrawCanvas_Rescale (src, dst, ext);
	UnlockMutex (tfbImg->mutex);
	if (destroy)
		DestroyDrawable (ReleaseDrawable (FramePtr));
	return (NewFrame);
}

void process_rgb_bmp (FRAMEPTR FramePtr, Uint32 *rgba, int maxx, int maxy)
{
	int x, y;
	TFB_Image *tfbImg;
	SDL_Surface *img;
	PutPixelFn putpix;

//	TYPE_SET (FramePtr->TypeIndexAndFlags, WANT_PIXMAP << FTYPE_SHIFT);
//	INDEX_SET (FramePtr->TypeIndexAndFlags,  1);


	// convert 32-bit png font to indexed
	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);

	putpix = putpixel_for (img);

	for (y = 0; y < maxy; ++y)
		for (x = 0; x < maxx; ++x)
			putpix (img, x, y, *rgba++);
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

void fill_frame_rgb (FRAMEPTR FramePtr, Uint32 color, int x0, int y0, int x, int y)
{
	SDL_Surface *img;
	TFB_Image *tfbImg;
	SDL_Rect rect;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);
	if (x0 == 0 && y0 == 0 && x == 0 && y == 0)
		SDL_FillRect(img, NULL, color);
	else
	{
		rect.x = x0;
		rect.y = y0;
		rect.w = x - x0;
		rect.h = y - y0;
		SDL_FillRect(img, &rect, color);
	}
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

void arith_frame_blit (FRAMEPTR srcFrame, RECT *rsrc, FRAMEPTR dstFrame, RECT *rdst, int num,int denom)
{
	TFB_Image *srcImg, *dstImg;
	SDL_Surface *src, *dst;
	SDL_Rect srcRect, dstRect, *srp = NULL, *drp = NULL;
	srcImg = srcFrame->image;
	dstImg = dstFrame->image;
	LockMutex (srcImg->mutex);
	LockMutex (dstImg->mutex);
	src = (SDL_Surface *)srcImg->NormalImg;
	dst = (SDL_Surface *)dstImg->NormalImg;
	if (rdst)
	{
		dstRect.x = rdst->corner.x;
		dstRect.y = rdst->corner.y;
		dstRect.w = rdst->extent.width;
		dstRect.h = rdst->extent.height;
		drp = &dstRect;
	}
	if (rsrc)
	{
		srcRect.x = rsrc->corner.x;
		srcRect.y = rsrc->corner.y;
		srcRect.w = rsrc->extent.width;
		srcRect.h = rsrc->extent.height;
		srp = &srcRect;
	}
	else if (srcFrame->HotSpot.x || srcFrame->HotSpot.y)
	{
		if (rdst)
		{
			dstRect.x -= srcFrame->HotSpot.x;
			dstRect.y -= srcFrame->HotSpot.y;
		}
		else
		{
			dstRect.x = -srcFrame->HotSpot.x;
			dstRect.y = -srcFrame->HotSpot.y;
			dstRect.w = GetFrameWidth (srcFrame);
			dstRect.h = GetFrameHeight (srcFrame);
			drp =&dstRect;
		}

	}
	TFB_BlitSurface (src, srp, dst, drp, num, denom);
	UnlockMutex (srcImg->mutex);
	UnlockMutex (dstImg->mutex);
}

// Generate an array of all pixels in FramePtr
// The pixel format is :
//  bits 25-32 : red
//  bits 17-24 : green
//  bits 9-16  : blue
//  bits 1-8   : alpha
Uint32 **getpixelarray(FRAMEPTR FramePtr, int width, int height)
{
	Uint8 r,g,b,a;
	Uint32 p, **map;
	TFB_Image *tfbImg;
	SDL_Surface *img;
	GetPixelFn getpix;
	int x,y;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);
	getpix = getpixel_for (img);
	map=(Uint32 **)HMalloc (sizeof(Uint32 *) * (height + 1));
	for (y = 0; y < height; y++)
	{
		map[y]=(Uint32 *)HCalloc (width * sizeof(Uint32));
		if(y >= img->h)
			continue;
		for(x = 0; x < img->w; x++)
		{
				p = getpix (img, x, y);
				SDL_GetRGBA (p,img->format, &r, &g, &b, &a);
				map[y][x] = r << 24 | g << 16 | b << 8 | a;
		}
	}
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
	map[y] = NULL;
	return (map);

}

FRAMEPTR Build_Font_Effect (FRAMEPTR FramePtr, Uint32 from, Uint32 to, BYTE type)
{
	UBYTE FrameType;
	FRAMEPTR NewFrame;
	int x, y;
	int width, height;
	Uint32 color, clear;
	BYTE from_r, from_g, from_b, from_a;
	BYTE to_r, to_g, to_b, to_a;
	TFB_Image *tfbImg, *tfbOrigImg;
	SDL_Surface *img, *OrigImg;
	PutPixelFn putpix;
	GetPixelFn getpix;

	width = GetFrameWidth (FramePtr);
	height = GetFrameHeight (FramePtr);
	FrameType = (UBYTE)TYPE_GET (GetFrameParentDrawable (FramePtr)
			->FlagsAndIndex) >> FTYPE_SHIFT;
	NewFrame = CaptureDrawable (
				CreateDrawable (FrameType, (SIZE)width, (SIZE)height, 1));
	tfbOrigImg = FramePtr->image;
	OrigImg = (SDL_Surface *)tfbOrigImg->NormalImg;
	SDL_LockSurface (OrigImg);

	tfbImg = NewFrame->image;
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);

	putpix = putpixel_for (img);
	getpix = getpixel_for (OrigImg);

	from_r = (from >> 24);
	from_g = (from >> 16) & 0xFF;
	from_b = (from >> 8) & 0xFF;
	from_a = from & 0xFF;

	to_r = (to >> 24);
	to_g = (to >> 16) & 0xFF;
	to_b = (to >> 8) & 0xFF;
	to_a = to & 0xFF;

	clear = SDL_MapRGBA (img->format, 0, 0, 0, 0);
	for (y = 0; y < 2; y++)
		for (x = 0; x < width; x++)
			putpix (img, x, y, clear);

	width--;
	if (type == GRADIENT_EFFECT)
	{
		int clrstep_r, clrstep_g, clrstep_b, clrstep_a;
		clrstep_r = (to_r - from_r) /  (height - 5);
		clrstep_g = (to_g - from_g) /  (height - 5);
		clrstep_b = (to_b - from_b) /  (height - 5);
		clrstep_a = (to_a - from_a) /  (height - 5);
		for (; y < height - 1; y++)
		{
			color = SDL_MapRGBA (img->format, from_r, from_g, from_b, from_a);
			from_r += clrstep_r;
			from_g += clrstep_g;
			from_b += clrstep_b;
			from_a += clrstep_a;
			for (x = 0; x < width; x++)
				if (getpix (OrigImg, x, y) == 1)
					putpix (img, x, y, color);
				else
					putpix (img, x, y, clear);
			putpix (img, x, y, clear);
		}
	}
	else if (type == ALTERNATE_EFFECT)
	{
		Uint32 color1, color2;
		color1 = SDL_MapRGBA (img->format, from_r, from_g, from_b, from_a);
		color2 = SDL_MapRGBA (img->format, to_r, to_g, to_b, to_a);
		for (; y < height - 1; y++)
		{
			for (x = 0; x < width; x++)
				if (getpix (OrigImg, x, y) == 1)
				{
					color = ((y & 0x01) ^ (x & 0x01)) ? color2 : color1;
					putpix (img, x, y, color);
				}
				else
					putpix (img, x, y, clear);
			putpix (img, x, y, clear);
		}
	}

	width++;
	for (x = 0; x < width; x++)
		putpix (img, x, y, clear);
	SDL_UnlockSurface (img);
	SDL_UnlockSurface (OrigImg);
	NewFrame->HotSpot = FramePtr->HotSpot;
	return (NewFrame);

}
// Generate a pixel (in the correct format to be applied to FramePtr) from the
// r,g,b,a values supplied
Uint32 frame_mapRGBA (FRAMEPTR FramePtr,Uint8 r, Uint8 g,  Uint8 b, Uint8 a)
{
	SDL_Surface *img= (SDL_Surface *)FramePtr->image->NormalImg;
	return (SDL_MapRGBA (img->format, r, g, b, a));
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
	AniData ani[MAX_CELS];
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
		/*char fnamestr[1000];
		sscanf(CurrentLine, "%s", fnamestr);
		fprintf (stderr, "imgload %s\n",fnamestr);*/
		
		sscanf (CurrentLine, "%s %d %d %d %d", &filename[n], 
			&ani[cel_ct].transparent_color, &ani[cel_ct].colormap_index, 
			&ani[cel_ct].hotspot_x, &ani[cel_ct].hotspot_y);
	
		if ((img[cel_ct] = IMG_Load (filename)) && img[cel_ct]->w > 0 && 
			img[cel_ct]->h > 0 && img[cel_ct]->format->BitsPerPixel >= 8) 
		{
			++cel_ct;
		}
		else if (img[cel_ct]) 
		{
			fprintf (stderr, "_GetCelData: Bad file!\n");
			SDL_FreeSurface (img[cel_ct]);
		}

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

			mem_release ((MEM_HANDLE)Drawable);
			Drawable = 0;
		}
		else
		{
			FRAMEPTR FramePtr;

			DrawablePtr->hDrawable = GetDrawableHandle (Drawable);
			TYPE_SET (DrawablePtr->FlagsAndIndex,
					(DRAWABLE_TYPE)WANT_PIXMAP << FTYPE_SHIFT);
			INDEX_SET (DrawablePtr->FlagsAndIndex, cel_ct - 1);

			FramePtr = &DrawablePtr->Frame[cel_ct];
			while (--FramePtr, cel_ct--)
				process_image (FramePtr, img, ani, cel_ct);

			UnlockDrawable (Drawable);
		}
	}

	if (Drawable == 0)
		fprintf (stderr, "Couldn't get cel data for '%s'\n",
				_cur_resfile_name);
	return (GetDrawableHandle (Drawable));
}

BOOLEAN
_ReleaseCelData (MEM_HANDLE handle)
{
	DRAWABLEPTR DrawablePtr;
	int cel_ct;
	FRAMEPTR FramePtr;

	if ((DrawablePtr = LockDrawable (handle)) == 0)
		return (FALSE);

	cel_ct = INDEX_GET (DrawablePtr->FlagsAndIndex)+1;

	FramePtr = &DrawablePtr->Frame[cel_ct];
	if (!(TYPE_GET ((FramePtr-1)->TypeIndexAndFlags) & SCREEN_DRAWABLE))
	{
		while (--FramePtr, cel_ct--)
		{
			TFB_Image *img = FramePtr->image;
			if (img)
			{
				FramePtr->image = NULL;
				TFB_DrawScreen_DeleteImage (img);
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
		if (sscanf (char_name, "%u.", (unsigned int*) &cel_ct) == 1
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
					process_font (FramePtr, img, cel_ct);
					SetFrameBounds (FramePtr, GetFrameWidth (FramePtr) + 1, GetFrameHeight (FramePtr) + 1);
					
					if (img[0])
					{
						// This tunes the font positioning to be about what it should
						// TODO: prolly needs a little tweaking still

						int tune_amount = 0;

						if (img[0]->h == 8)
							tune_amount = -1;
						else if (img[0]->h == 9)
							tune_amount = -2;
						else if (img[0]->h > 9)
							tune_amount = -3;

						FramePtr->HotSpot = MAKE_HOT_SPOT (0, img[0]->h + tune_amount);
					}
					
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
			TFB_Image *img = FramePtr->image;
			if (img)
			{
				FramePtr->image = NULL;

				TFB_DrawScreen_DeleteImage (img);
			}
		}
	}

	UnlockFont (handle);
	mem_release (handle);

	return (TRUE);
}

// _request_drawable was in libs/graphics/drawable.c before modularization

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

			imgw = width;
			imgh = height;
			
			FramePtr = &DrawablePtr->Frame[NumFrames - 1];
			while (NumFrames--)
			{
				TFB_Image *Image;

				if (DrawableType == RAM_DRAWABLE
						&& (Image = TFB_DrawImage_New (TFB_DrawCanvas_New_TrueColor (imgw, imgh, FALSE))))
				{
					FramePtr->image = Image;
				}

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
