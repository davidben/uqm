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

#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound_common.h"
#include "libs/input/input_common.h"

int
main (int argc, char *argv[])
{
	int i;
	int gfxdriver = TFB_GFXDRIVER_SDL_PURE;
	int gfxflags = 0;
	int width = 640, height = 480, bpp = 16;
	int frequency = 44100;

	// TODO: proper commandline parser, this is just a quick hack
	for (i=1;i<argc;++i) 
	{
		if (!strcmp(argv[i],"-res")) 
		{
			i++;
			sscanf(argv[i],"%dx%d",&width,&height);
		}
		else if (!strcmp(argv[i],"-bpp")) 
		{
			i++;
			sscanf(argv[i],"%d",&bpp);
		}
		else if (!strcmp(argv[i],"-fullscreen")) 
		{
			gfxflags |= TFB_GFXFLAGS_FULLSCREEN;
		}
		else if (!strcmp(argv[i],"-opengl")) 
		{
			gfxdriver = TFB_GFXDRIVER_SDL_OPENGL;
			gfxflags |= TFB_GFXFLAGS_BILINEAR_FILTERING;
		}
		else if (!strcmp(argv[i],"-linear")) 
		{
			gfxflags &= ~TFB_GFXFLAGS_BILINEAR_FILTERING;
		}
		else if (!strcmp(argv[i],"-frequency"))
		{
			i++;
			sscanf(argv[i],"%d",&frequency);
		}
	}

	TFB_InitGraphics (gfxdriver, gfxflags, width, height, bpp);
	TFB_InitSound (TFB_SOUNDDRIVER_SDL, 0, frequency);
	TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);

	TFB_CreateGamePlayThread ();

	for (;;)
	{
		TFB_ProcessEvents ();
		TFB_FlushGraphics ();
	}

	return 0;
}
