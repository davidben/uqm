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
#include "options.h"


void
CDToContentDir (char *contentdir)
{
    char *testfile = "starcon.txt";
    
    if (!FileExists (testfile))
    {
        if (chdir (contentdir) || !FileExists (testfile))
        {
			if (chdir ("content") || !FileExists (testfile))
			{
				fprintf(stderr, "Fatal error: content not available, running from wrong dir?\n");
				exit(-1);
			}
        }
    }
}

int
main (int argc, char *argv[])
{
	int gfxdriver = TFB_GFXDRIVER_SDL_PURE;
	int gfxflags = 0;
	int soundflags = TFB_SOUNDFLAGS_MQAUDIO;
	int width = 640, height = 480, bpp = 16;
	int vol;
	char contentdir[1000];
	char str[1000];

	int option_index = 0, c;
	static struct option long_options[] = 
	{
		{"res", 1, NULL, 'r'},
		{"bpp", 1, NULL, 'd'},
		{"fullscreen", 0, NULL, 'f'},
		{"opengl", 0, NULL, 'o'},
		{"bilinear", 0, NULL, 'b'},
		{"fps", 0, NULL, 'p'},
		{"tv", 0, NULL, 't'},
		{"contentdir", 1, NULL, 'c'},
		{"help", 0, NULL, 'h'},
		{"musicvol", 1, NULL, 'M'},
		{"sfxvol", 1, NULL, 'S'},
		{"speechvol", 1, NULL, 'T'},
        {"3domusic", 0, NULL, 'e'},
        {"pcmusic", 0, NULL, 'm'},
		{"audioquality", 1, NULL, 'q'},
		{0, 0, 0, 0}
	};

	fprintf (stderr, "The Ur-Quan Masters v%d.%d (compiled %s %s)\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION, __DATE__, __TIME__);

#ifndef WIN32
	strcpy (contentdir, "content");
#else
	strcpy (contentdir, "../../content");
#endif

	while ((c = getopt_long(argc, argv, "r:d:fob:ptc:?hM:S:T:emq:", long_options, &option_index)) != -1)
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
			case 'p':
				gfxflags |= TFB_GFXFLAGS_SHOWFPS;
			break;
			case 't':
				gfxflags |= TFB_GFXFLAGS_TVEFFECT;
			break;
			case 'c':
				sscanf(optarg, "%s", contentdir);
			break;
			case 'M':
				sscanf(optarg, "%d", &vol);
				if (vol < 0)
					vol = 0;
				if (vol > 100)
					vol = 100;
				musicVolumeScale = vol / 100.0f;
			break;
			case 'S':
				sscanf(optarg, "%d", &vol);
				if (vol < 0)
					vol = 0;
				if (vol > 100)
					vol = 100;
				sfxVolumeScale = vol / 100.0f;
			break;
			case 'T':
				sscanf(optarg, "%d", &vol);
				if (vol < 0)
					vol = 0;
				if (vol > 100)
					vol = 100;
				speechVolumeScale = vol / 100.0f;
			break;
            case 'e':
                optWhichMusic = MUSIC_3DO;
            break;
            case 'm':
                optWhichMusic = MUSIC_PC;
            break;
			case 'q':
				sscanf(optarg, "%s", str);
				if (!strcmp (str, "high"))
				{
					soundflags &= ~(TFB_SOUNDFLAGS_MQAUDIO|TFB_SOUNDFLAGS_LQAUDIO);
					soundflags |= TFB_SOUNDFLAGS_HQAUDIO;
				}
				else if (!strcmp (str, "medium"))
				{
					soundflags &= ~(TFB_SOUNDFLAGS_HQAUDIO|TFB_SOUNDFLAGS_LQAUDIO);
					soundflags |= TFB_SOUNDFLAGS_MQAUDIO;
				}
				else if (!strcmp (str, "low"))
				{
					soundflags &= ~(TFB_SOUNDFLAGS_HQAUDIO|TFB_SOUNDFLAGS_MQAUDIO);
					soundflags |= TFB_SOUNDFLAGS_LQAUDIO;
				}
			break;
			default:
				printf ("\nOption %s not found!\n", long_options[option_index].name);
			case '?':
			case 'h':
				printf("\nOptions:\n");
				printf("  -r, --res=WIDTHxHEIGHT (default 640x480, others work only with --opengl)\n");
				printf("  -d, --bpp=BITSPERPIXEL (default 16)\n");
				printf("  -f, --fullscreen (default off)\n");
				printf("  -o, --opengl (default off, usage recommended if TNT2 or better gfx card)\n");
				printf("  -b, --bilinear (default off, only works with --opengl)\n");
				printf("  -p, --fps (default off)\n");
				printf("  -t, --tv (default off, only works with --opengl\n");
				printf("  -c, --contentdir=CONTENTDIR\n");
				printf("  -M, --musicvol=VOLUME (0-100, default 100)\n");
				printf("  -S, --sfxvol=VOLUME (0-100, default 100)\n");
				printf("  -T, --speechvol=VOLUME (0-100, default 100)\n");
                printf("  -e, --3domusic (default)\n");
                printf("  -m, --pcmusic\n");
				printf("  -q, --audioquality=QUALITY (high, medium or low, default medium)\n");
				return 0;
			break;
		}
	}
	
	CDToContentDir (contentdir);
    
	InitThreadSystem ();
	InitTimeSystem ();
	InitTaskSystem ();

	_MemorySem = CreateSemaphore (1);
	GraphicsSem = CreateSemaphore (1);
	mem_init ();

	TFB_InitGraphics (gfxdriver, gfxflags, width, height, bpp);
	TFB_InitSound (TFB_SOUNDDRIVER_SDL, soundflags);
	TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);

	AssignTask (Starcon2Main, 1024, "Starcon2Main");

	for (;;)
	{
		TFB_ProcessEvents ();
		TFB_FlushGraphics ();
	}

	exit(0);
}
