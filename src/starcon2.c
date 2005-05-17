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
#	include <getopt.h>
#else
#	include "getopt/getopt.h"
#endif

#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound.h"
#include "libs/input/input_common.h"
#include "libs/tasklib.h"
#include "file.h"
#include "port.h"
#include "options.h"
#include "uqmversion.h"
#include "comm.h"
#include "setup.h"
#include "starcon.h"


#if defined(GFXMODULE_SDL)
#	include SDL_INCLUDE(SDL.h)
			// Including this is actually necessary on OSX.
#endif

struct options_struct {
	const char *logFile;
	enum {
		runMode_normal,
		runMode_usage,
		runMode_version,
	} runMode;
	int gfxDriver;
	int gfxFlags;
	int soundDriver;
	int soundFlags;
	int width;
	int height;
	int bpp;
	const char *configDir;
	const char *contentDir;
	const char **addons;
	int numAddons;
	int gammaSet;
	float gamma;
	int whichMusic;
	int whichCoarseScan;
	int whichMenu;
	int whichFonts;
	int whichIntro;
	int smoothScroll;
	int meleeScale;
	BOOLEAN subTitles;
	BOOLEAN stereoSFX;
	float musicVolumeScale;
	float sfxVolumeScale;
	float speechVolumeScale;
};

static int preParseOptions(int argc, char *argv[],
		struct options_struct *options);
static int parseOptions(int argc, char *argv[],
		struct options_struct *options);
static void usage (FILE *out, const struct options_struct *defaultOptions);
static int parseIntOption (const char *str, int *result,
		const char *optName);
static int parseFloatOption (const char *str, float *f,
		const char *optName);
static int parseVolume (const char *str, float *vol, const char *optName);
static int InvalidArgument(const char *supplied, const char *opt_name);
static int Check_PC_3DO_opt (const char *value, DWORD mask, const char *opt,
		int *result);
static const char *PC_3DO_optString (DWORD optMask);

/* TODO: Remove these global variables once threading is gone. */
int snddriver, soundflags;

int
main (int argc, char *argv[])
{
	struct options_struct options = {
		/* .logFile = */            NULL,
		/* .runMode = */            runMode_normal,
		/* .gfxdriver = */          TFB_GFXDRIVER_SDL_PURE,
		/* .gfxflags = */           0,
		/* .soundDriver = */        audio_DRIVER_MIXSDL,
		/* .soundFlags = */         audio_QUALITY_MEDIUM,
		/* .width = */              640,
		/* .height = */             480,
		/* .bpp = */                16,
		/* .configDir = */          NULL,
		/* .contentDir = */         NULL,
		/* .addons = */             NULL,
		/* .numAddons = */          0,
		/* .gammaSet = */           0,
		/* .gamma = */              0.0f,
		/* .whichMusic = */         OPT_3DO,
		/* .whichCoarseScan = */    OPT_PC,
		/* .whichMenu = */          OPT_PC,
		/* .whichFont = */          OPT_PC,
		/* .whichIntro = */         OPT_3DO,
		/* .smoothScroll = */       OPT_PC,
		/* .meleeScale = */         TFB_SCALE_TRILINEAR,
		/* .subtitles = */          TRUE,
		/* .stereoSFX = */          FALSE,
		/* .musicVolumeScale = */   1.0f,
		/* .sfxVolumeScale = */     1.0f,
		/* .speechVolumeScale = */  1.0f,
	};

	int optionsResult;
	optionsResult = preParseOptions(argc, argv, &options);
	if (optionsResult != 0)
	{
		// TODO: various uninitialisations
		return optionsResult;
	}

	if (options.logFile != NULL)
	{
		int i;
		freopen (options.logFile, "w", stderr);
		for (i = 0; i < argc; ++i)
			fprintf (stderr, "argv[%d] = [%s]\n", i, argv[i]);
	}

	if (options.runMode == runMode_version)
	{
 		printf ("%d.%d.%d%s\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
				UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
		return EXIT_SUCCESS;
	}
	
	fprintf (stderr, "The Ur-Quan Masters v%d.%d.%d%s (compiled %s %s)\n"
	        "This software comes with ABSOLUTELY NO WARRANTY;\n"
			"for details see the included 'COPYING' file.\n\n",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION, UQM_EXTRA_VERSION,
			__DATE__, __TIME__);

	if (options.runMode == runMode_usage)
	{
		usage (stdout, &options);
		return EXIT_SUCCESS;
	}

	/* mem_init () uses mutexes.  Mutex creation cannot use
	   the memory system until the memory system is rewritten
	   to rely on a thread-safe allocator.
	*/
	TFB_PreInit ();
	mem_init ();
	InitThreadSystem ();

	optionsResult = parseOptions(argc, argv, &options);
	if (optionsResult != 0)
	{
		// TODO: various uninitialisations
		return optionsResult;
	}


	/* TODO: Once threading is gone, these become local variables
	   again.  In the meantime, they must be global so that
	   initAudio (in StarCon2Main) can see them.  initAudio needed
	   to be moved there because calling AssignTask in the main
	   thread doesn't work */
	snddriver = options.soundDriver;
	soundflags = options.soundFlags;

	// Fill in global variables:
	optWhichMusic = options.whichMusic;
	optWhichCoarseScan = options.whichCoarseScan;
	optWhichMenu = options.whichMenu;
	optWhichFonts = options.whichFonts;
	optWhichIntro = options.whichIntro;
	optSmoothScroll = options.smoothScroll;
	optMeleeScale = options.meleeScale;
	optSubtitles = options.subTitles;
	optStereoSFX = options.stereoSFX;
	musicVolumeScale = options.musicVolumeScale;
	sfxVolumeScale = options.sfxVolumeScale;
	speechVolumeScale = options.speechVolumeScale;

	initIO ();
	prepareContentDir (options.contentDir, options.addons);
	HFree ((void *) options.addons);
	prepareConfigDir (options.configDir);
	prepareMeleeDir ();
	prepareSaveDir ();
	initTempDir ();

	initLanguage ();

	InitTimeSystem ();
	InitTaskSystem ();

	GraphicsLock = CreateMutex ("Graphics",
			SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);

	TFB_InitGraphics (options.gfxDriver, options.gfxFlags,
			options.width, options.height, options.bpp);
	if (options.gammaSet)
		TFB_SetGamma (options.gamma);
	init_communication ();
	/* TODO: Once threading is gone, restore initAudio here.
	   initAudio calls AssignTask, which currently blocks on
	   ProcessThreadLifecycles... */
	// initAudio (snddriver, soundflags);
	TFB_InitInput (TFB_INPUTDRIVER_SDL, 0);

	StartThread (Starcon2Main, NULL, 1024, "Starcon2Main");

	for (;;)
	{
		TFB_ProcessEvents ();
		ProcessThreadLifecycles ();
		TFB_FlushGraphics ();
	}

	unInitTempDir ();
	uninitIO ();
	exit (EXIT_SUCCESS);
}

enum
{
	CSCAN_OPT = 1000,
	MENU_OPT,
	FONT_OPT,
	SCROLL_OPT,
	SOUND_OPT,
	STEREOSFX_OPT,
	ADDON_OPT
};

static const char *optString = "+r:d:foc:b:spC:n:?hM:S:T:m:q:ug:l:i:v";
static struct option longOptions[] = 
{
	{"res", 1, NULL, 'r'},
	{"bpp", 1, NULL, 'd'},
	{"fullscreen", 0, NULL, 'f'},
	{"opengl", 0, NULL, 'o'},
	{"scale", 1, NULL, 'c'},
	{"meleezoom", 1, NULL, 'b'},
	{"scanlines", 0, NULL, 's'},
	{"fps", 0, NULL, 'p'},
	{"configdir", 1, NULL, 'C'},
	{"contentdir", 1, NULL, 'n'},
	{"help", 0, NULL, 'h'},
	{"musicvol", 1, NULL, 'M'},
	{"sfxvol", 1, NULL, 'S'},
	{"speechvol", 1, NULL, 'T'},
	{"audioquality", 1, NULL, 'q'},
	{"nosubtitles", 0, NULL, 'u'},
	{"music", 1, NULL, 'm'},
	{"gamma", 1, NULL, 'g'},
	{"logfile", 1, NULL, 'l'},
	{"intro", 1, NULL, 'i'},
	{"version", 0, NULL, 'v'},

	//  options with no short equivalent:
	{"cscan", 1, NULL, CSCAN_OPT},
	{"menu", 1, NULL, MENU_OPT},
	{"font", 1, NULL, FONT_OPT},
	{"scroll", 1, NULL, SCROLL_OPT},
	{"sound", 1, NULL, SOUND_OPT},
	{"stereosfx", 0, NULL, STEREOSFX_OPT},
	{"addon", 1, NULL, ADDON_OPT},
	{0, 0, 0, 0}
};

static int
preParseOptions(int argc, char *argv[], struct options_struct *options)
{
	/*
	 *	"pre-process" the cmdline args looking for a -l ("logfile")
	 *	option.  If it was given, redirect stderr to the named file.
	 *	Also handle the switches were normal operation is inhibited.
	 */
	opterr = 0;
	for (;;)
	{
		int c = getopt_long (argc, argv, optString, longOptions, 0);
		if (c == -1)
			break;

		switch (c)
		{
			case 'l':
			{
				options->logFile = optarg;
				break;
			}
			case '?':
			case 'h':
				options->runMode = runMode_usage;
				return EXIT_SUCCESS;
			case 'v':
				options->runMode = runMode_version;
				return EXIT_SUCCESS;
		}
	}
	optind = 1;
	return 0;
}

static int
parseOptions(int argc, char *argv[], struct options_struct *options)
{
	int optionIndex = 0;
	BOOLEAN badArg = 0;

	options->addons = HMalloc(1 * sizeof (const char *));
	options->addons[0] = NULL;
	options->numAddons = 0;

	if (argc == 0)
	{
		fprintf (stderr, "Error: Bad command line.\n");
		return EXIT_FAILURE;
	}

	while (!badArg)
	{
		int c;
		c = getopt_long(argc, argv, optString, longOptions, &optionIndex);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				options->configDir = optarg;
				break;
			case 'r':
			{
				int width, height;
				if (sscanf (optarg, "%dx%d", &width, &height) != 2)
				{
					fprintf (stderr, "Error: invalid argument specified "
							"as resolution.\n");
					badArg = TRUE;
					break;
				}
				options->width = width;
				options->height = height;
				break;
			}
			case 'd':
			{
				int bpp;
				int err = parseIntOption(optarg, &bpp, "bpp");
				if (err)
				{
					badArg = TRUE;
					break;
				}
				if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
				{
					fprintf (stderr, "Error: Invalid value specified for "
							"bpp; it should be either 8, 16, 24, or 32.\n");
					badArg = TRUE;
					break;
				}
				options->bpp = bpp;
				break;
			}
			case 'f':
				options->gfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
				break;
			case 'o':
				options->gfxDriver = TFB_GFXDRIVER_SDL_OPENGL;
				break;
			case 'c':
				if (!strcmp (optarg, "bilinear"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_BILINEAR;
				else if (!strcmp (optarg, "biadapt"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPT;
				else if (!strcmp (optarg, "biadv"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPTADV;
				else if (!strcmp (optarg, "triscan"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_TRISCAN;
				else
				{
					InvalidArgument(optarg, "--scale or -c");
					badArg = TRUE;
				}
				break;
			case 'b':
				if (!strcmp (optarg, "smooth") || !strcmp (optarg, "3do"))
					options->meleeScale = TFB_SCALE_TRILINEAR;
				else if (!strcmp (optarg, "step") || !strcmp (optarg, "pc"))
					options->meleeScale = TFB_SCALE_STEP;
				else
				{
					InvalidArgument(optarg, "--meleezoom or -b");
					badArg = TRUE;
				}
				break;
			case 's':
				options->gfxFlags |= TFB_GFXFLAGS_SCANLINES;
				break;
			case 'p':
				options->gfxFlags |= TFB_GFXFLAGS_SHOWFPS;
				break;
			case 'n':
				options->contentDir = optarg;
				break;
			case 'M':
			{
				int err = parseVolume(optarg, &options->musicVolumeScale,
						"music volume");
				if (err)
					badArg = TRUE;
				break;
			}
			case 'S':
			{
				int err = parseVolume(optarg, &options->sfxVolumeScale,
						"sfx volume");
				if (err)
					badArg = TRUE;
				break;
			}
			case 'T':
			{
				int err = parseVolume(optarg, &options->speechVolumeScale,
						"speech volume");
				if (err)
					badArg = TRUE;
				break;
			}
			case 'q':
				if (!strcmp (optarg, "high"))
				{
					options->soundFlags &=
							~(audio_QUALITY_MEDIUM | audio_QUALITY_LOW);
					options->soundFlags |= audio_QUALITY_HIGH;
				}
				else if (!strcmp (optarg, "medium"))
				{
					options->soundFlags &=
							~(audio_QUALITY_MEDIUM | audio_QUALITY_LOW);
					options->soundFlags |= audio_QUALITY_MEDIUM;
				}
				else if (!strcmp (optarg, "low"))
				{
					options->soundFlags &=
							~(audio_QUALITY_MEDIUM | audio_QUALITY_LOW);
					options->soundFlags |= audio_QUALITY_LOW;
				}
				else
				{
					InvalidArgument(optarg, "--audioquality or -q");
					badArg = TRUE;
				}
				break;
			case 'u':
				options->subTitles = FALSE;
				break;
			case 'm':
			{
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->whichMusic) == -1)
					badArg = TRUE;
				break;
			}
			case 'g':
			{
				int err = parseFloatOption(optarg, &options->gamma,
						"gamma correction");
				if (err)
					badArg = TRUE;
				else
					options->gammaSet = TRUE;
				break;
			}
			case 'l':
				// -l is a no-op on the second pass..
				break;
			case 'i':
			{
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->whichIntro) == -1)
					badArg = TRUE;
				break;
			}
			case CSCAN_OPT:
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->whichCoarseScan) == -1)
					badArg = TRUE;
				break;
			case MENU_OPT:
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->whichMenu) == -1)
					badArg = TRUE;
				break;
			case FONT_OPT:
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->whichFonts) == -1)
					badArg = TRUE;
				break;
			case SCROLL_OPT:
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->smoothScroll) == -1)
					badArg = TRUE;
				break;
			case SOUND_OPT:
				if (!strcmp (optarg, "openal"))
					options->soundDriver = audio_DRIVER_OPENAL;
				else if (!strcmp (optarg, "mixsdl"))
					options->soundDriver = audio_DRIVER_MIXSDL;
				else if (!strcmp (optarg, "none"))
				{
					options->soundDriver = audio_DRIVER_NOSOUND;
					options->speechVolumeScale = 0.0f;
				}
				else
				{
					fprintf (stderr, "Error: Invalid sound driver "
							"specified.\n");
					badArg = TRUE;
				}
				break;
			case STEREOSFX_OPT:
				options->stereoSFX = TRUE;
				break;
			case ADDON_OPT:
				options->numAddons++;
				options->addons = HRealloc ((void *) options->addons,
						(options->numAddons + 1) * sizeof (const char *));
				options->addons[options->numAddons - 1] = optarg;
				options->addons[options->numAddons] = NULL;
				break;
			default:
				fprintf (stderr, "Error: Invalid option '%s' not found.\n",
							longOptions[optionIndex].name);
				badArg = TRUE;
				break;
			break;
		}
	}

	if (optind != argc)
	{
		fprintf (stderr, "\nError: Extra arguments found on the command "
				"line.\n");
		badArg = TRUE;
	}

	if (badArg)
	{
		fprintf (stderr, "Run with -h to see the allowed arguments.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
parseVolume (const char *str, float *vol, const char *optName)
{
	char *endPtr;
	int intVol;

	if (str[0] == '\0')
	{
		fprintf (stderr, "Error: Invalid value for '%s'.\n", optName);
		return -1;
	}
	intVol = (int) strtol(str, &endPtr, 10);
	if (*endPtr != '\0')
	{
		fprintf (stderr, "Error: Junk characters in volume specified "
				"for '%s'.\n", optName);
		return -1;
	}

	if (intVol < 0)
	{
		*vol = 0.0f;
		return 0;
	}

	if (intVol > 100)
	{
		*vol = 1.0f;
		return 0;
	}

	*vol = intVol / 100.0f;
	return 0;
}

static int
parseIntOption (const char *str, int *result, const char *optName)
{
	char *endPtr;
	int temp;

	if (str[0] == '\0')
	{
		fprintf (stderr, "Error: Invalid value for '%s'.\n", optName);
		return -1;
	}
	temp = (int) strtol(str, &endPtr, 10);
	if (*endPtr != '\0')
	{
		fprintf (stderr, "Error: Junk characters in argument '%s'.\n",
				optName);
		return -1;
	}

	*result = temp;
	return 0;
}

static int
parseFloatOption (const char *str, float *f, const char *optName)
{
	char *endPtr;
	float temp;

	if (str[0] == '\0')
	{
		fprintf (stderr, "Error: Invalid value for '%s'.\n", optName);
		return -1;
	}
	temp = (float) strtod(str, &endPtr);
	if (*endPtr != '\0')
	{
		fprintf (stderr, "Error: Junk characters in argument '%s'.\n",
				optName);
		return -1;
	}

	*f = temp;
	return 0;
}

static void
usage (FILE *out, const struct options_struct *defaultOptions)
{
	fprintf (out, "Options:\n");
	fprintf (out, "  -r, --res=WIDTHxHEIGHT (default 640x480, bigger "
			"works only with --opengl)\n");
	fprintf (out, "  -d, --bpp=BITSPERPIXEL (default 16)\n");
	fprintf (out, "  -f, --fullscreen (default off)\n");
	fprintf (out, "  -o, --opengl (default off)\n");
	fprintf (out, "  -c, --scale=MODE (bilinear, biadapt, biadv or triscan, "
			"default is none)\n");
	fprintf (out, "  -b, --meleezoom=MODE (step, aka pc, or smooth, aka 3do; "
			"default is 3do)\n");
	fprintf (out, "  -s, --scanlines (default off)\n");
	fprintf (out, "  -p, --fps (default off)\n");
	fprintf (out, "  -g, --gamma=CORRECTIONVALUE (default 1.0, which "
			"causes no change)\n");
	fprintf (out, "  -C, --configdir=CONFIGDIR\n");
	fprintf (out, "  -n, --contentdir=CONTENTDIR\n");
	fprintf (out, "  -M, --musicvol=VOLUME (0-100, default 100)\n");
	fprintf (out, "  -S, --sfxvol=VOLUME (0-100, default 100)\n");
	fprintf (out, "  -T, --speechvol=VOLUME (0-100, default 100)\n");
	fprintf (out, "  -q, --audioquality=QUALITY (high, medium or low, "
			"default medium)\n");
	fprintf (out, "  -u, --nosubtitles\n");
	fprintf (out, "  -l, --logfile=FILE (sends console output to logfile "
			"FILE)\n");
	fprintf (out, "  --addon ADDON (using a specific addon; "
			"may be specified multiple times)\n");
	fprintf (out, "  --sound=DRIVER (openal, mixsdl, none; default "
			"mixsdl)\n");
	fprintf (out, "  --stereosfx (enables positional sound effects, "
			"currently only for openal)\n");
	fprintf (out, "The following options can take either '3do' or 'pc' "
			"as an option:\n");
	fprintf (out, "  -m, --music : Music version (default %s)\n",
			PC_3DO_optString(defaultOptions->whichMusic));
	fprintf (out, "  -i, --intro : Intro/ending version (default %s)\n",
			PC_3DO_optString(defaultOptions->whichIntro));
	fprintf (out, "  --cscan     : coarse-scan display, pc=text, "
			"3do=hieroglyphs (default %s)\n",
			PC_3DO_optString(defaultOptions->whichCoarseScan));
	fprintf (out, "  --menu      : menu type, pc=text, 3do=graphical "
			"(default %s)\n", PC_3DO_optString(defaultOptions->whichMenu));
	fprintf (out, "  --font      : font types and colors (default %s)\n",
			PC_3DO_optString(defaultOptions->whichFonts));
	fprintf (out, "  --scroll    : ff/frev during comm.  pc=per-page, "
			"3do=smooth (default %s)\n",
			PC_3DO_optString(defaultOptions->smoothScroll));
}

static int
InvalidArgument(const char *supplied, const char *opt_name)
{
	fprintf(stderr, "Invalid argument '%s' to option %s.\n",
			supplied, opt_name);
	fprintf (stderr, "Use -h to see the allowed arguments.\n");
	return EXIT_FAILURE;
}

static int
Check_PC_3DO_opt (const char *value, DWORD mask, const char *optName,
		int *result)
{
	if (value == NULL)
	{
		fprintf (stderr, "Error: option '%s' requires a value.\n", optName);
		return -1;
	}

	if ((mask & OPT_3DO) && strcmp (value, "3do") == 0)
	{
		*result = OPT_3DO;
		return 0;
	}
	if ((mask & OPT_PC) && strcmp (value, "pc") == 0)
	{
		*result = OPT_PC;
		return 0;
	}
	fprintf (stderr, "Error: Invalid option '%s %s' found.", optName, value);
	return -1;
}

static const char *
PC_3DO_optString(DWORD optMask) {
	if (optMask & OPT_3DO)
	{
		if (optMask & OPT_PC)
			return "both";
		return "3do";
	}
	else
	{
		if (optMask & OPT_PC)
			return "pc";
		return "none";
	}
}


