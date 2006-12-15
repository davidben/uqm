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

#ifdef HAVE_GETOPT_LONG
#	include <getopt.h>
#else
#	include "getopt/getopt.h"
#endif

#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound.h"
#include "libs/input/input_common.h"
#include "libs/tasklib.h"
#include "controls.h"
#include "file.h"
#include "port.h"
#include "libs/platform.h"
#include "libs/log.h"
#include "options.h"
#include "uqmversion.h"
#include "comm.h"
#ifdef NETPLAY
#	include "libs/callback.h"
#	include "libs/alarm.h"
#	include "libs/net.h"
#	include "netplay/netoptions.h"
#	include "netplay/netplay.h"
#endif
#include "setup.h"
#include "starcon.h"


#if defined (GFXMODULE_SDL)
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
	int whichShield;
	int smoothScroll;
	int meleeScale;
	BOOLEAN subTitles;
	BOOLEAN stereoSFX;
	float musicVolumeScale;
	float sfxVolumeScale;
	float speechVolumeScale;
};

static int preParseOptions (int argc, char *argv[],
		struct options_struct *options);
static int parseOptions (int argc, char *argv[],
		struct options_struct *options);
static void usage (FILE *out, const struct options_struct *defaultOptions);
static int parseIntOption (const char *str, int *result,
		const char *optName);
static int parseFloatOption (const char *str, float *f,
		const char *optName);
static int parseVolume (const char *str, float *vol, const char *optName);
static int InvalidArgument (const char *supplied, const char *opt_name);
static int Check_PC_3DO_opt (const char *value, DWORD mask, const char *opt,
		int *result);
static const char *PC_3DO_optString (DWORD optMask);

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
		/* .whichShield	= */        OPT_PC,
		/* .smoothScroll = */       OPT_PC,
		/* .meleeScale = */         TFB_SCALE_TRILINEAR,
		/* .subtitles = */          TRUE,
		/* .stereoSFX = */          FALSE,
		/* .musicVolumeScale = */   1.0f,
		/* .sfxVolumeScale = */     1.0f,
		/* .speechVolumeScale = */  1.0f,
	};
	int optionsResult;

	log_init (15);

	optionsResult = preParseOptions (argc, argv, &options);
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
			log_add (log_User, "argv[%d] = [%s]", i, argv[i]);
	}

	if (options.runMode == runMode_version)
	{
 		printf ("%d.%d.%d%s\n", UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
				UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
		log_showBox (false, false);
		return EXIT_SUCCESS;
	}
	
	log_add (log_User, "The Ur-Quan Masters v%d.%d.%d%s (compiled %s %s)\n"
	        "This software comes with ABSOLUTELY NO WARRANTY;\n"
			"for details see the included 'COPYING' file.\n",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION, UQM_EXTRA_VERSION,
			__DATE__, __TIME__);
#ifdef NETPLAY
	log_add (log_User, "Netplay protocol version %d.%d. Requiring remote "
			"UQM version %d.%d.%d.",
			NETPLAY_PROTOCOL_VERSION_MAJOR, NETPLAY_PROTOCOL_VERSION_MINOR,
			NETPLAY_MIN_UQM_VERSION_MAJOR, NETPLAY_MIN_UQM_VERSION_MINOR,
			NETPLAY_MIN_UQM_VERSION_PATCH);
#endif

	if (options.runMode == runMode_usage)
	{
		usage (stdout, &options);
		log_showBox (true, false);
		return EXIT_SUCCESS;
	}

	/* mem_init () uses mutexes.  Mutex creation cannot use
	   the memory system until the memory system is rewritten
	   to rely on a thread-safe allocator.
	*/
	TFB_PreInit ();
	mem_init ();
	InitThreadSystem ();
	log_initThreads ();
	initIO ();
	prepareConfigDir (options.configDir);

	PlayerOne = CONTROL_TEMPLATE_KB_1;
	PlayerTwo = CONTROL_TEMPLATE_JOY_1;

	// Fill in the options struct based on uqm.cfg
	res_LoadFilename (configDir, "uqm.cfg");
	if (res_HasKey ("config.reswidth"))
	{
		options.width = res_GetInteger ("config.reswidth");
	}
	if (res_HasKey ("config.resheight"))
	{
		options.height = res_GetInteger ("config.resheight");
	}
	if (res_HasKey ("config.alwaysgl"))
	{
		if (res_GetBoolean ("config.alwaysgl"))
		{
			options.gfxDriver = TFB_GFXDRIVER_SDL_OPENGL;
		}
	}
	if (res_HasKey ("config.usegl"))
	{
		options.gfxDriver = res_GetBoolean ("config.usegl") ? TFB_GFXDRIVER_SDL_OPENGL :
			TFB_GFXDRIVER_SDL_PURE;
	}
	if (res_HasKey ("config.scaler"))
	{
		const char *optarg = res_GetString ("config.scaler");

		if (!strcmp (optarg, "bilinear"))
			options.gfxFlags |= TFB_GFXFLAGS_SCALE_BILINEAR;
		else if (!strcmp (optarg, "biadapt"))
			options.gfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPT;
		else if (!strcmp (optarg, "biadv"))
			options.gfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPTADV;
		else if (!strcmp (optarg, "triscan"))
			options.gfxFlags |= TFB_GFXFLAGS_SCALE_TRISCAN;
		else if (!strcmp (optarg, "hq"))
			options.gfxFlags |= TFB_GFXFLAGS_SCALE_HQXX;	
	}
	if (res_HasKey ("config.scanlines") && res_GetBoolean ("config.scanlines"))
	{
		options.gfxFlags |= TFB_GFXFLAGS_SCANLINES;
	}
	if (res_HasKey ("config.fullscreen") && res_GetBoolean ("config.fullscreen"))
	{
		options.gfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
	}
	if (res_HasKey ("config.subtitles"))
	{
		options.subTitles = res_GetBoolean ("config.subtitles");
	}
	if (res_HasKey ("config.textmenu"))
	{
		options.whichMenu = res_GetBoolean ("config.textmenu") ? OPT_PC : OPT_3DO;
	}
	if (res_HasKey ("config.textgradients"))
	{
		options.whichFonts = res_GetBoolean ("config.textgradients") ? OPT_PC : OPT_3DO;
	}
	if (res_HasKey ("config.iconicscan"))
	{
		options.whichCoarseScan = res_GetBoolean ("config.iconicscan") ? OPT_3DO : OPT_PC;
	}
	if (res_HasKey ("config.smoothscroll"))		
	{
		options.smoothScroll = res_GetBoolean ("config.smoothscroll") ? OPT_3DO : OPT_PC;
	}
	if (res_HasKey ("config.3domusic"))
	{
		options.whichMusic = res_GetBoolean ("config.3domusic") ? OPT_3DO : OPT_PC;
	}
	if (res_HasKey ("config.3domovies"))
	{
		options.whichIntro = res_GetBoolean ("config.3domovies") ? OPT_3DO : OPT_PC;
	}
	if (res_HasKey ("config.showfps") && res_GetBoolean ("config.showfps"))
	{
		options.gfxFlags |= TFB_GFXFLAGS_SHOWFPS;
	}
	if (res_HasKey ("config.smoothmelee"))
	{
		options.meleeScale = res_GetBoolean ("config.smoothmelee") ? TFB_SCALE_TRILINEAR : TFB_SCALE_STEP;
	}
	if (res_HasKey ("config.positionalsfx"))
	{
		options.stereoSFX = res_GetBoolean ("config.positionalsfx");
	}
	if (res_HasKey ("config.audiodriver"))
	{
		const char *driverstr = res_GetString ("config.audiodriver");
		if (!strcmp (driverstr, "openal"))
		{
			options.soundDriver = audio_DRIVER_OPENAL;
		}
		else if (!strcmp (driverstr, "none"))
		{
			options.soundDriver = audio_DRIVER_NOSOUND;
			options.speechVolumeScale = 0.0f;
		}
		else if (!strcmp (driverstr, "mixsdl"))
		{
			options.soundDriver = audio_DRIVER_MIXSDL;
		}
		else
		{
			/* Can't figure it out, leave as initial default */
		}
	}
	if (res_HasKey ("config.audioquality"))
	{
		const char *qstr = res_GetString ("config.audioquality");
		if (!strcmp (qstr, "low"))
		{
			options.soundFlags &=
					~(audio_QUALITY_MEDIUM | audio_QUALITY_HIGH);
			options.soundFlags |= audio_QUALITY_LOW;
		}
		else if (!strcmp (qstr, "medium"))
		{
			options.soundFlags &=
					~(audio_QUALITY_HIGH | audio_QUALITY_LOW);
			options.soundFlags |= audio_QUALITY_MEDIUM;
		}
		else if (!strcmp (qstr, "high"))
		{
			options.soundFlags &=
					~(audio_QUALITY_MEDIUM | audio_QUALITY_LOW);
			options.soundFlags |= audio_QUALITY_HIGH;
		}
		else
		{
			/* Can't figure it out, leave as initial default */
		}
	}
	if (res_HasKey ("config.pulseshield"))
	{
		options.whichShield = res_GetBoolean ("config.pulseshield") ? OPT_3DO : OPT_PC;
	}
	if (res_HasKey ("config.player1control"))
	{
		PlayerOne = res_GetInteger ("config.player1control");
		/* This is an unsigned, so no < 0 check is necessary */
		if (PlayerOne >= NUM_TEMPLATES)
		{
			log_add (log_Error, "Illegal control template '%d' for Player One.", PlayerOne);
			PlayerOne = CONTROL_TEMPLATE_KB_1;
		}
	}
	if (res_HasKey ("config.player2control"))
	{
		/* This is an unsigned, so no < 0 check is necessary */
		PlayerTwo = res_GetInteger ("config.player2control");
		if (PlayerTwo >= NUM_TEMPLATES)
		{
			log_add (log_Error, "Illegal control template '%d' for Player Two.", PlayerTwo);
			PlayerTwo = CONTROL_TEMPLATE_JOY_1;
		}
	}
	if (res_HasKey ("config.musicvol"))
	{
		parseVolume (res_GetString ("config.musicvol"), 
				&options.musicVolumeScale, "music volume");
	}		
	if (res_HasKey ("config.sfxvol"))
	{
		parseVolume (res_GetString ("config.sfxvol"), 
				&options.sfxVolumeScale, "SFX volume");
	}		
	if (res_HasKey ("config.speechvol"))
	{
		parseVolume (res_GetString ("config.speechvol"), 
				&options.speechVolumeScale, "speech volume");
	}		

	{	/* init control template names */
		static const char* defaultNames[] =
		{
			"Arrows", "WASD", "Arrows (2)",
			"ESDF", "Joystick 1", "Joystick 2"
		};
		int i;

		for (i = 0; i < 6; ++i)
		{
			char cfgkey[64];

			snprintf(cfgkey, sizeof(cfgkey), "config.keys.%d.name", i + 1);
			cfgkey[sizeof(cfgkey) - 1] = '\0';

			if (res_HasKey (cfgkey))
			{
				strncpy (input_templates[i].name, res_GetString (cfgkey), 30);
				input_templates[i].name[29] = 0;
			}
			else
			{
				strcpy (input_templates[i].name, defaultNames[i]);
				res_PutString (cfgkey, input_templates[i].name);
			}
		}
	}

	optionsResult = parseOptions (argc, argv, &options);
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
	optWhichShield = options.whichShield;
	optSmoothScroll = options.smoothScroll;
	optMeleeScale = options.meleeScale;
	optSubtitles = options.subTitles;
	optStereoSFX = options.stereoSFX;
	musicVolumeScale = options.musicVolumeScale;
	sfxVolumeScale = options.sfxVolumeScale;
	speechVolumeScale = options.speechVolumeScale;

	prepareContentDir (options.contentDir, options.addons);
	HFree ((void *) options.addons);
	prepareMeleeDir ();
	prepareSaveDir ();
	initTempDir ();

	InitTimeSystem ();
	InitTaskSystem ();

#ifdef NETPLAY
	Network_init ();
	Alarm_init ();
	Callback_init ();
	NetManager_init ();
#endif

	GraphicsLock = CreateMutex ("Graphics",
			SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);

	TFB_InitGraphics (options.gfxDriver, options.gfxFlags,
			options.width, options.height);
	if (options.gammaSet)
		TFB_SetGamma (options.gamma);
	InitColorMaps ();
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

#ifdef NETPLAY
	Network_uninit ();
#endif
	
	return EXIT_SUCCESS;
}

enum
{
	CSCAN_OPT = 1000,
	MENU_OPT,
	FONT_OPT,
	SHIELD_OPT,
	SCROLL_OPT,
	SOUND_OPT,
	STEREOSFX_OPT,
	ADDON_OPT,
	ACCEL_OPT,
#ifdef NETPLAY
	NETHOST1_OPT,
	NETPORT1_OPT,
	NETHOST2_OPT,
	NETPORT2_OPT,
	NETDELAY_OPT,
#endif
};

static const char *optString = "+r:d:foc:b:spC:n:?hM:S:T:m:q:ug:l:i:v";
static struct option longOptions[] = 
{
	{"res", 1, NULL, 'r'},
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
	{"shield", 1, NULL, SHIELD_OPT},
	{"scroll", 1, NULL, SCROLL_OPT},
	{"sound", 1, NULL, SOUND_OPT},
	{"stereosfx", 0, NULL, STEREOSFX_OPT},
	{"addon", 1, NULL, ADDON_OPT},
	{"accel", 1, NULL, ACCEL_OPT},
#ifdef NETPLAY
	{"nethost1", 1, NULL, NETHOST1_OPT},
	{"netport1", 1, NULL, NETPORT1_OPT},
	{"nethost2", 1, NULL, NETHOST2_OPT},
	{"netport2", 1, NULL, NETPORT2_OPT},
	{"netdelay", 1, NULL, NETDELAY_OPT},
#endif
	{0, 0, 0, 0}
};

static int
preParseOptions (int argc, char *argv[], struct options_struct *options)
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
			case 'C':
			{
				options->configDir = optarg;
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
parseOptions (int argc, char *argv[], struct options_struct *options)
{
	int optionIndex;
	BOOLEAN badArg = FALSE;

	options->addons = HMalloc (1 * sizeof (const char *));
	options->addons[0] = NULL;
	options->numAddons = 0;

	if (argc == 0)
	{
		log_add (log_Fatal, "Error: Bad command line.");
		return EXIT_FAILURE;
	}

	while (!badArg)
	{
		int c;
		optionIndex = -1;
		c = getopt_long (argc, argv, optString, longOptions, &optionIndex);
		if (c == -1)
			break;

		switch (c) {
			case 'r':
			{
				int width, height;
				if (sscanf (optarg, "%dx%d", &width, &height) != 2)
				{
					log_add (log_Fatal, "Error: invalid argument specified "
							"as resolution.");
					badArg = TRUE;
					break;
				}
				options->width = width;
				options->height = height;
				break;
			}
			case 'f':
				options->gfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
				break;
			case 'o':
				options->gfxDriver = TFB_GFXDRIVER_SDL_OPENGL;
				break;
			case 'c':
				// make sure whatever was set by saved config is cleared
				options->gfxFlags &= ~TFB_GFXFLAGS_SCALE_ANY;
				if (!strcmp (optarg, "bilinear"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_BILINEAR;
				else if (!strcmp (optarg, "biadapt"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPT;
				else if (!strcmp (optarg, "biadv"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPTADV;
				else if (!strcmp (optarg, "triscan"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_TRISCAN;
				else if (!strcmp (optarg, "hq"))
					options->gfxFlags |= TFB_GFXFLAGS_SCALE_HQXX;
				else if (strcmp (optarg, "none") != 0)
				{
					InvalidArgument (optarg, "--scale or -c");
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
					InvalidArgument (optarg, "--meleezoom or -b");
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
				int err = parseVolume (optarg, &options->musicVolumeScale,
						"music volume");
				if (err)
					badArg = TRUE;
				break;
			}
			case 'S':
			{
				int err = parseVolume (optarg, &options->sfxVolumeScale,
						"sfx volume");
				if (err)
					badArg = TRUE;
				break;
			}
			case 'T':
			{
				int err = parseVolume (optarg, &options->speechVolumeScale,
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
					InvalidArgument (optarg, "--audioquality or -q");
					badArg = TRUE;
				}
				break;
			case 'u':
				options->subTitles = FALSE;
				break;
			case 'm':
			{
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						optionIndex >= 0 ? longOptions[optionIndex].name : "m",
						&options->whichMusic) == -1)
					badArg = TRUE;
				break;
			}
			case 'g':
			{
				int result = parseFloatOption (optarg, &options->gamma,
						"gamma correction");
				if (result == -1)
					badArg = TRUE;
				else
					options->gammaSet = TRUE;
				break;
			}
			case 'l':
			case 'C':
				// -l and -C are no-ops on the second pass.
				break;			
			case 'i':
			{
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						optionIndex >= 0 ? longOptions[optionIndex].name : "i",
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
			case SHIELD_OPT:
				if (Check_PC_3DO_opt (optarg, OPT_PC | OPT_3DO,
						longOptions[optionIndex].name,
						&options->whichShield) == -1)
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
					log_add (log_Fatal, "Error: Invalid sound driver "
							"specified.");
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
			case ACCEL_OPT:
				force_platform = PLATFORM_NULL;
				if (!strcmp (optarg, "mmx"))
					force_platform = PLATFORM_MMX;
				else if (!strcmp (optarg, "sse"))
					force_platform = PLATFORM_SSE;
				else if (!strcmp (optarg, "3dnow"))
					force_platform = PLATFORM_3DNOW;
				else if (!strcmp (optarg, "none"))
					force_platform = PLATFORM_C;
				else if (strcmp (optarg, "detect") != 0)
				{
					InvalidArgument (optarg, "--accel");
					badArg = TRUE;
				}
				break;
#ifdef NETPLAY
			case NETHOST1_OPT:
				netplayOptions.peer[0].isServer = false;
				netplayOptions.peer[0].host = optarg;
				break;
			case NETPORT1_OPT:
				netplayOptions.peer[0].port = optarg;
				break;
			case NETHOST2_OPT:
				netplayOptions.peer[1].isServer = false;
				netplayOptions.peer[1].host = optarg;
				break;
			case NETPORT2_OPT:
				netplayOptions.peer[1].port = optarg;
				break;
			case NETDELAY_OPT:
			{
				if (parseIntOption (optarg, &netplayOptions.inputDelay,
						"network input delay") == -1)
				{
					badArg = TRUE;
					break;
				}

				if (netplayOptions.inputDelay > 60 * BATTLE_FRAME_RATE)
				{
					log_add (log_Fatal, "Network input delay is absurdly "
							"large.");
					badArg = TRUE;
				}
				break;
			}
#endif
			default:
				log_add (log_Fatal, "Error: Invalid option '%s' not found.",
							longOptions[optionIndex].name);
				badArg = TRUE;
				break;
		}
	}

	if (optind != argc)
	{
		log_add (log_Fatal, "\nError: Extra arguments found on the command "
				"line.");
		badArg = TRUE;
	}

	if (badArg)
	{
		log_add (log_Fatal, "Run with -h to see the allowed arguments.");
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
		log_add (log_Error, "Error: Invalid value for '%s'.", optName);
		return -1;
	}
	intVol = (int) strtol (str, &endPtr, 10);
	if (*endPtr != '\0')
	{
		log_add (log_Error, "Error: Junk characters in volume specified "
				"for '%s'.", optName);
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
		log_add (log_Error, "Error: Invalid value for '%s'.", optName);
		return -1;
	}
	temp = (int) strtol (str, &endPtr, 10);
	if (*endPtr != '\0')
	{
		log_add (log_Error, "Error: Junk characters in argument '%s'.",
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
		log_add (log_Error, "Error: Invalid value for '%s'.", optName);
		return -1;
	}
	temp = (float) strtod (str, &endPtr);
	if (*endPtr != '\0')
	{
		log_add (log_Error, "Error: Junk characters in argument '%s'.",
				optName);
		return -1;
	}

	*f = temp;
	return 0;
}

static void
usage (FILE *out, const struct options_struct *defaultOptions)
{
	FILE *old = log_setOutput (out);
	log_captureLines (LOG_CAPTURE_ALL);
	
	log_add (log_User, "Options:");
	log_add (log_User, "  -r, --res=WIDTHxHEIGHT (default 640x480, bigger "
			"works only with --opengl)");
	log_add (log_User, "  -f, --fullscreen (default off)");
	log_add (log_User, "  -o, --opengl (default off)");
	log_add (log_User, "  -c, --scale=MODE (bilinear, biadapt, biadv, triscan, "
			"hq or none (default) )");
	log_add (log_User, "  -b, --meleezoom=MODE (step, aka pc, or smooth, aka 3do; "
			"default is 3do)");
	log_add (log_User, "  -s, --scanlines (default off)");
	log_add (log_User, "  -p, --fps (default off)");
	log_add (log_User, "  -g, --gamma=CORRECTIONVALUE (default 1.0, which "
			"causes no change)");
	log_add (log_User, "  -C, --configdir=CONFIGDIR");
	log_add (log_User, "  -n, --contentdir=CONTENTDIR");
	log_add (log_User, "  -M, --musicvol=VOLUME (0-100, default 100)");
	log_add (log_User, "  -S, --sfxvol=VOLUME (0-100, default 100)");
	log_add (log_User, "  -T, --speechvol=VOLUME (0-100, default 100)");
	log_add (log_User, "  -q, --audioquality=QUALITY (high, medium or low, "
			"default medium)");
	log_add (log_User, "  -u, --nosubtitles");
	log_add (log_User, "  -l, --logfile=FILE (sends console output to logfile "
			"FILE)");
	log_add (log_User, "  --addon ADDON (using a specific addon; "
			"may be specified multiple times)");
	log_add (log_User, "  --sound=DRIVER (openal, mixsdl, none; default "
			"mixsdl)");
	log_add (log_User, "  --stereosfx (enables positional sound effects, "
			"currently only for openal)");
#ifdef NETPLAY
	log_add (log_User, "  --nethostN=HOSTNAME (server to connect to for "
			"player N (1=bottom, 2=top)");
	log_add (log_User, "  --netportN=PORT (port to connect to/listen on for "
			"player N (1=bottom, 2=top)");
	log_add (log_User, "  --netdelay=FRAMES (number of frames to "
			"buffer/delay network input for");
#endif
	log_add (log_User, "The following options can take either '3do' or 'pc' "
			"as an option:");
	log_add (log_User, "  -m, --music : Music version (default %s)",
			PC_3DO_optString (defaultOptions->whichMusic));
	log_add (log_User, "  -i, --intro : Intro/ending version (default %s)",
			PC_3DO_optString (defaultOptions->whichIntro));
	log_add (log_User, "  --cscan     : coarse-scan display, pc=text, "
			"3do=hieroglyphs (default %s)",
			PC_3DO_optString (defaultOptions->whichCoarseScan));
	log_add (log_User, "  --menu      : menu type, pc=text, 3do=graphical "
			"(default %s)", PC_3DO_optString(defaultOptions->whichMenu));
	log_add (log_User, "  --font      : font types and colors (default %s)",
			PC_3DO_optString (defaultOptions->whichFonts));
	log_add (log_User, "  --shield    : slave shield type; pc=static, "
			"3do=throbbing (default %s)",
			PC_3DO_optString (defaultOptions->whichShield));
	log_add (log_User, "  --scroll    : ff/frev during comm.  pc=per-page, "
			"3do=smooth (default %s)",
			PC_3DO_optString (defaultOptions->smoothScroll));
	log_setOutput (old);
}

static int
InvalidArgument (const char *supplied, const char *opt_name)
{
	log_add (log_Fatal, "Invalid argument '%s' to option %s.",
			supplied, opt_name);
	return EXIT_FAILURE;
}

static int
Check_PC_3DO_opt (const char *value, DWORD mask, const char *optName,
		int *result)
{
	if (value == NULL)
	{
		log_add (log_Error, "Error: option '%s' requires a value.",
				optName);
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
	log_add (log_Error, "Error: Invalid option '%s %s' found.",
			optName, value);
	return -1;
}

static const char *
PC_3DO_optString (DWORD optMask)
{
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


