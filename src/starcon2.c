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

#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#ifdef WIN32
#	include <direct.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt/getopt.h"
#endif

#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound_common.h"
#include "libs/input/input_common.h"
#include "libs/tasklib.h"
#include "file.h"
#include "port.h"
#include "options.h"

#if defined(GFXMODULE_SDL) || defined(SOUNDMODULE_SDL)
#	include <SDL.h>
			// Including this is actually necessary on OSX.
#endif

static void
CDToContentDir (char *contentdir)
{
	const char *testfile = "starcon.txt";

	if (!fileExists (testfile))
	{
		if ((chdir (contentdir) || !fileExists (testfile)) &&
				(chdir ("content") || !fileExists (testfile)) &&
				(chdir ("../../content") || !fileExists (testfile))) {
			fprintf(stderr, "Fatal error: content not available, running from wrong dir?\n");
			exit(EXIT_FAILURE);
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

	int option_index = 0, c;
	static struct option long_options[] = 
	{
		{"res", 1, NULL, 'r'},
		{"bpp", 1, NULL, 'd'},
		{"fullscreen", 0, NULL, 'f'},
		{"opengl", 0, NULL, 'o'},
		{"scale", 1, NULL, 'c'},
		{"scanlines", 0, NULL, 's'},
		{"fps", 0, NULL, 'p'},
		{"contentdir", 1, NULL, 'n'},
		{"help", 0, NULL, 'h'},
		{"musicvol", 1, NULL, 'M'},
		{"sfxvol", 1, NULL, 'S'},
		{"speechvol", 1, NULL, 'T'},
		{"3domusic", 0, NULL, 'e'},
		{"pcmusic", 0, NULL, 'm'},
		{"audioquality", 1, NULL, 'q'},
		{"nosubtitles", 0, NULL, 'u'},
		{"pcscan", 0, NULL, 'a'},
		{0, 0, 0, 0}
	};

	fprintf (stderr, "The Ur-Quan Masters v%d.%d (compiled %s %s)\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION, __DATE__, __TIME__);

#ifdef CONTENTDIR
	strcpy (contentdir, CONTENTDIR);
#elif defined (win32)
	strcpy (contentdir, "../../content");
#else
	strcpy (contentdir, "content");
#endif

	while ((c = getopt_long(argc, argv, "r:d:foc:spn:?hM:S:T:emq:ua", long_options, &option_index)) != -1)
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
			case 'c':
				if (!strcmp (optarg, "bilinear"))
				{
					gfxflags |= TFB_GFXFLAGS_SCALE_BILINEAR;
				}
				else if (!strcmp (optarg, "biadapt"))
				{
					gfxflags |= TFB_GFXFLAGS_SCALE_BIADAPT;
				}
			break;
			case 's':
				gfxflags |= TFB_GFXFLAGS_SCANLINES;
			break;
			case 'p':
				gfxflags |= TFB_GFXFLAGS_SHOWFPS;
			break;
			case 'n':
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
				if (!strcmp (optarg, "high"))
				{
					soundflags &= ~(TFB_SOUNDFLAGS_MQAUDIO|TFB_SOUNDFLAGS_LQAUDIO);
					soundflags |= TFB_SOUNDFLAGS_HQAUDIO;
				}
				else if (!strcmp (optarg, "medium"))
				{
					soundflags &= ~(TFB_SOUNDFLAGS_HQAUDIO|TFB_SOUNDFLAGS_LQAUDIO);
					soundflags |= TFB_SOUNDFLAGS_MQAUDIO;
				}
				else if (!strcmp (optarg, "low"))
				{
					soundflags &= ~(TFB_SOUNDFLAGS_HQAUDIO|TFB_SOUNDFLAGS_MQAUDIO);
					soundflags |= TFB_SOUNDFLAGS_LQAUDIO;
				}
			break;
		    case 'u':
			    optSubtitles = FALSE;
			break;
			case 'a':
				optPCscan = TRUE;
			break;
			default:
				printf ("\nOption %s not found!\n", long_options[option_index].name);
			case '?':
			case 'h':
				printf("\nOptions:\n");
				printf("  -r, --res=WIDTHxHEIGHT (default 640x480, bigger works only with --opengl)\n");
				printf("  -d, --bpp=BITSPERPIXEL (default 16)\n");
				printf("  -f, --fullscreen (default off)\n");
				printf("  -o, --opengl (default off)\n");
				printf("  -c, --scale=mode (bilinear or biadapt, default is none)\n");
				printf("  -s, --scanlines (default off)\n");
				printf("  -p, --fps (default off)\n");
				printf("  -n, --contentdir=CONTENTDIR\n");
				printf("  -M, --musicvol=VOLUME (0-100, default 100)\n");
				printf("  -S, --sfxvol=VOLUME (0-100, default 100)\n");
				printf("  -T, --speechvol=VOLUME (0-100, default 100)\n");
				printf("  -e, --3domusic (default)\n");
				printf("  -m, --pcmusic\n");
				printf("  -q, --audioquality=QUALITY (high, medium or low, default medium)\n");
			    printf("  -u, --nosubtitles\n");
				printf("  -a  --pcscan\n");
				return 0;
			break;
		}
	}
	
	initTempDir();
	CDToContentDir (contentdir);
	prepareConfigDir();
	prepareMeleeDir();
	prepareSaveDir();
	
	InitThreadSystem ();
	InitTimeSystem ();
	InitTaskSystem ();

	mem_init ();
	GraphicsSem = CreateSemaphore (1, "Graphics");
	RenderingCond = CreateCondVar ();
	init_xform_control ();

	TFB_InitGraphics (gfxdriver, gfxflags, width, height, bpp);
	TFB_InitSound (TFB_SOUNDDRIVER_SDL, soundflags);
	TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);

	AssignTask (Starcon2Main, 1024, "Starcon2Main");

	for (;;)
	{
		TFB_ProcessEvents ();
		TFB_FlushGraphics ();
	}

	unInitTempDir();
	exit(EXIT_SUCCESS);
}

