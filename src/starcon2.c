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

#ifndef WIN32
#include <unistd.h>
#include <getopt.h>
#else
#include <direct.h>
#include "getopt/getopt.h"
#endif

#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound_common.h"
#include "libs/input/input_common.h"
#include "libs/tasklib.h"

BOOLEAN FileExists (char *filename)
{
    FILE *fp;
    if ((fp = fopen(filename, "rb")) == NULL)
        return FALSE;

    fclose (fp);
    return TRUE;
}

void
CDToContentDir (char *contentdir)
{
    char *testfile = "starcon.txt";
    
    if (!FileExists (testfile))
    {
        if (chdir(contentdir) || !FileExists (testfile))
        {
            fprintf(stderr, "Fatal error: content not available, running from wrong dir?\n");
            exit(-1);
        }
    }
}

int
main (int argc, char *argv[])
{
	int gfxdriver = TFB_GFXDRIVER_SDL_PURE;
	int gfxflags = 0;
	int width = 640, height = 480, bpp = 16;
	int frequency = 44100;
	char contentdir[1000];

	int option_index = 0, c;
	static struct option long_options[] = 
	{
		{"res", 1, NULL, 'r'},
		{"bpp", 1, NULL, 'd'},
		{"fullscreen", 0, NULL, 'f'},
		{"opengl", 0, NULL, 'o'},
		{"bilinear", 0, NULL, 'b'},
		{"frequency", 1, NULL, 'q'},
		{"fps", 0, NULL, 'p'},
		{"tv", 0, NULL, 't'},
		{"contentdir", 1, NULL, 'c'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0}
	};

#ifndef WIN32
	strcpy (contentdir, "content");
#else
	strcpy (contentdir, "../../content");
#endif

	while ((c = getopt_long(argc, argv, "r:d:fobq:ptc:?h", long_options, &option_index)) != -1)
	{
		switch (c) {
			case 'r':
				sscanf(optarg, "%dx%d", &width, &height);
			break;
			case 'd':
				sscanf(optarg, "%d", &bpp);
			break;
			case 'f':
				gfxflags |= TFB_GFXFLAGS_FULLSCREEN;
			break;
			case 'o':
				gfxdriver = TFB_GFXDRIVER_SDL_OPENGL;
			break;
			case 'b':
				gfxflags |= TFB_GFXFLAGS_BILINEAR_FILTERING;
			break;
			case 'q':
				sscanf(optarg, "%d", &frequency);
			break;
			case 'p':
				gfxflags |= TFB_GFXFLAGS_SHOWFPS;
			break;
			case 't':
				gfxflags |= TFB_GFXFLAGS_TVEFFECT;
			break;
			case 'c':
				sscanf(optarg, "%s", contentdir);
			break;
			default:
				printf ("\nOption %s not found!\n", long_options[option_index].name);
			case '?':
			case 'h':
				printf("\nThe Ur-Quan Masters\n");
				printf("Options:\n");
				printf("  -r, --res=WIDTHxHEIGHT\n");
				printf("  -d, --bpp=BITSPERPLANE\n");
				printf("  -f, --fullscreen\n");
				printf("  -o, --opengl\n");
				printf("  -b, --bilinear\n");
				printf("  -q, --frequency=FREQUENCY\n");
				printf("  -p, --fps\n");
				printf("  -t, --tv\n");
				printf("  -c, --contentdir=CONTENTDIR\n");
				return 0;
			break;
		}
	}
	
	CDToContentDir (contentdir);
    
	InitThreadSystem ();
	InitTimeSystem ();
	InitTaskSystem ();
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
