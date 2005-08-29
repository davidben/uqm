// Copyright Michael Martin, 2004.

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

#include "setupmenu.h"

#include "controls.h"
#include "options.h"
#include "setup.h"
#include "sounds.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/widgets.h"
#include "libs/graphics/tfb_draw.h"

#define MENU_BKG "lbm/setupmenu.ani"

typedef struct setup_menu_state {
	BOOLEAN (*InputFunc) (struct setup_menu_state *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN initialized;
	int anim_frame_count;
	DWORD NextTime;
} SETUP_MENU_STATE;

typedef SETUP_MENU_STATE *PSETUP_MENU_STATE;

static BOOLEAN DoSetupMenu (PSETUP_MENU_STATE pInputState);
static BOOLEAN done;

static int
quit_main_menu (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		done = TRUE;
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static CHOICE_OPTION scaler_opts[] = {
	{ "None",
	  { "No scaling.",
	    "Simulates the original 320x240 display.", 
	    "Fastest, though least attractive, option.",
	  } },
	{ "Bilinear",
	  { "Bilinear average scaling.",
	    "A simple, fast blending technique.",
	    "Hardware-accelerated in OpenGL mode."
	  } },
	{ "Biadapt",
	  { "Adaptive bilinear scaling.",
	    "A slower, less blurry blending technique.",
	    ""
	  } },
	{ "Biadv",
	  { "Advanced bilinear scaling.",
	    "Hand-weighted blend that produces smoother curves.",
	    "The most expensive scaler."
	  } },
	{ "Triscan",
	  { "An interpolating scaler.",
	    "Produces sharp edges, but at a higher resolution.",
	    "Based on the Scale2x algorithm.",
	  } } };

static CHOICE_OPTION scanlines_opts[] = {
	{ "Disabled",
		{ "Do not attempt to simulate an interlaced display.",
		  "",
		  ""
		} },
	{ "Enabled",
		{ "Simulate an interlaced display.",
		  "",
		  ""
		} } };

static CHOICE_OPTION music_opts[] = {
	{ "PC",
		{ "Uses the music from the Original PC version.",
		  "Prefers .MOD files to .OGG files.",
		  ""
		} },
	{ "3do/Remixes",
		{ "Uses the music from the 3do version and/or the",
		  "Ur-Quan Masters Remix Project, if available.",
		  "Prefers .OGG files over .MOD."
		} } };

static CHOICE_OPTION menu_opts[] = {
	{ "Text",
		{ "In-game menus resemble the Original PC version.",
		  "",
		  ""
		} },
	{ "Pictographic",
		{ "In-game menus resemble the 3do version.",
		  "",
		  ""
		} } };

static CHOICE_OPTION font_opts[] = {
	{ "Gradients",
		{ "Certain menu texts and dialogs use gradients,",
		  "as per the original PC version.",
		  "Looks nicer."
		} },
	{ "Flat",
		{ "All text and menus use a \"flat\" colour scheme,",
		  "as per the 3do version.",
		  "Easier to read."
		} } };

static CHOICE_OPTION scan_opts[] = {
	{ "Text",
		{ "Displays planet scan information as text,",
		  "as per the original PC version.",
		  ""
		} },
	{ "Pictograms",
		{ "Displays planet scan information as pictograms,",
		  "as per the 3do version..",
		  ""
		} } };

static CHOICE_OPTION scroll_opts[] = {
	{ "Per-Page",
		{ "When fast-forwarding or rewinding conversations",
		  "in-game, advance one screen of subtitles at a time.",
		  "This mimics the Original PC version."
		} },
	{ "Smooth",
		{ "When fast-forwarding or rewinding conversations",
		  "in-game, advance at a linear rate.",
		  "This mimics the 3do version."
		} } };

static CHOICE_OPTION subtitles_opts[] = {
	{ "Disabled",
		{ "Do not subtitle alien speech.",
		  "",
		  ""
		} },
	{ "Enabled",
		{ "Show subtitles for alien speech.",
		  "",
		  ""
		} } };

static CHOICE_OPTION resdriver_opts[] = {
	{ "320x240",
		{ "320x240 resolution.",
		  "Uses the SDL frame buffer directly.",
		  ""
		} },
	{ "640x480",
		{ "640x480 resolution.",
		  "Uses the SDL frame buffer directly.",
		  ""
		} },
#ifdef HAVE_OPENGL
	{ "320x240 GL",
		{ "320x240 resolution.",
		  "Uses OpenGL graphics drivers.",
		  ""
		} },
	{ "640x480 GL",
		{ "640x480 resolution.",
		  "Uses OpenGL graphics drivers.",
		  ""
		} },
	{ "800x600 GL",
		{ "800x600 resolution.",
		  "Uses OpenGL graphics drivers.",
		  ""
		} },
	{ "1024x768 GL",
		{ "1024x768 resolution.",
		  "Uses OpenGL graphics drivers.",
		  ""
		} },
	{ "Custom",
		{ "Custom resolution set from the commandline.",
		  "Uses OpenGL graphics drivers.",
		  ""
		} },
#endif
};

static CHOICE_OPTION bpp_opts[] = {
	{ "16",
		{ "16-bit color depth.",
		  "",
		  ""
		} },
	{ "24",
		{ "24-bit color depth.",
		  "",
		  ""
		} },
	{ "32",
		{ "32-bit color depth.",
		  "",
		  ""
		} } };

#ifdef HAVE_OPENGL
#define RES_OPTS 6
#else
#define RES_OPTS 2
#endif

#define NUM_OPTS 9

#define CHOICE_PREFACE NULL, Widget_HandleEventChoice, Widget_ReceiveFocusChoice, Widget_DrawChoice, Widget_HeightChoice, Widget_WidthFullScreen

static WIDGET_CHOICE cmdline_opts[] = {
	{ CHOICE_PREFACE, "Resolution", RES_OPTS, resdriver_opts, 0, 0},
	{ CHOICE_PREFACE, "Color depth", 3, bpp_opts, 0, 0 },
	{ CHOICE_PREFACE, "Scaler", 5, scaler_opts, 0, 0 },
  	{ CHOICE_PREFACE, "Scanlines", 2, scanlines_opts, 0, 0 },
	{ CHOICE_PREFACE, "Menu Style", 2, menu_opts, 0, 0 },
	{ CHOICE_PREFACE, "Font Style", 2, font_opts, 0, 0 },
	{ CHOICE_PREFACE, "Scan Style", 2, scan_opts, 0, 0 },
	{ CHOICE_PREFACE, "Scroll Style", 2, scroll_opts, 0, 0 },
	{ CHOICE_PREFACE, "Subtitles", 2, subtitles_opts, 0, 0 } };

static WIDGET_BUTTON quit_button = {
	NULL, quit_main_menu, Widget_ReceiveFocusButton, Widget_DrawButton,
	Widget_HeightOneLine, Widget_WidthFullScreen,
	"Quit Setup Menu", {"Return to the main menu.", "", ""} };

static WIDGET *opt_widgets[] = {
	(WIDGET *)(&cmdline_opts[0]),
	(WIDGET *)(&cmdline_opts[1]),
	(WIDGET *)(&cmdline_opts[2]),
	(WIDGET *)(&cmdline_opts[3]),
	(WIDGET *)(&cmdline_opts[4]),
	(WIDGET *)(&cmdline_opts[5]),
	(WIDGET *)(&cmdline_opts[6]),
	(WIDGET *)(&cmdline_opts[7]),
	(WIDGET *)(&cmdline_opts[8]),
	(WIDGET *)(&quit_button) } ;

static WIDGET_MENU_SCREEN menu = {
	NULL, Widget_HandleEventMenuScreen, Widget_ReceiveFocusMenuScreen,
	Widget_DrawMenuScreen, Widget_HeightFullScreen, Widget_WidthFullScreen,
	"Ur-Quan Masters Setup",
	"",
	{ {0, 0}, NULL },
	10, opt_widgets,
	0 };	

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (ONE_SECOND / 20)

static void
SetDefaults (void)
{
	GLOBALOPTS opts;
	
	GetGlobalOptions (&opts);
	if (opts.driver == OPTVAL_CUSTOM_GL)
	{
		cmdline_opts[0].numopts = RES_OPTS + 1;
	}
	else
	{
		cmdline_opts[0].numopts = RES_OPTS;
	}
	cmdline_opts[0].selected = opts.driver;
	cmdline_opts[1].selected = opts.depth;
	cmdline_opts[2].selected = opts.scaler;
	cmdline_opts[3].selected = opts.scanlines;
	cmdline_opts[4].selected = opts.menu;
	cmdline_opts[5].selected = opts.text;
	cmdline_opts[6].selected = opts.cscan;
	cmdline_opts[7].selected = opts.scroll;
	cmdline_opts[8].selected = opts.subtitles;
}

static void
PropagateResults (void)
{
	GLOBALOPTS opts;
	opts.driver = cmdline_opts[0].selected;
	opts.depth = cmdline_opts[1].selected;
	opts.scaler = cmdline_opts[2].selected;
	opts.scanlines = cmdline_opts[3].selected;
	opts.menu = cmdline_opts[4].selected;
	opts.text = cmdline_opts[5].selected;
	opts.cscan = cmdline_opts[6].selected;
	opts.scroll = cmdline_opts[7].selected;
	opts.subtitles = cmdline_opts[8].selected;

	SetGlobalOptions (&opts);
}

static BOOLEAN
DoSetupMenu (PSETUP_MENU_STATE pInputState)
{
	if (!pInputState->initialized) 
	{
		SetDefaultMenuRepeatDelay ();
		SetTransitionSource (NULL);
		pInputState->NextTime = GetTimeCounter ();
		SetDefaults ();
		(*menu.receiveFocus) ((WIDGET *)(&menu), WIDGET_EVENT_DOWN);
	}
	if (!menu.bgStamp.frame)
	{
		menu.bgStamp.origin.x = 0;
		menu.bgStamp.origin.y = 0;
		menu.bgStamp.frame = CaptureDrawable (LoadCelFile (MENU_BKG));
	}
	
	BatchGraphics ();
	(*menu.draw)((WIDGET *)(&menu), 0, 0);

	if (PulsedInputState.key[KEY_MENU_UP])
	{
		Widget_Event (WIDGET_EVENT_UP);
	}
	else if (PulsedInputState.key[KEY_MENU_DOWN])
	{
		Widget_Event (WIDGET_EVENT_DOWN);
	}
	else if (PulsedInputState.key[KEY_MENU_LEFT])
	{
		Widget_Event (WIDGET_EVENT_LEFT);
	}
	else if (PulsedInputState.key[KEY_MENU_RIGHT])
	{
		Widget_Event (WIDGET_EVENT_RIGHT);
	}
	if (PulsedInputState.key[KEY_MENU_SELECT])
	{
		Widget_Event (WIDGET_EVENT_SELECT);
	}

	if (!pInputState->initialized) 
	{
		pInputState->initialized = TRUE;
		ScreenTransition (3, NULL);
	}
	UnbatchGraphics ();
	SleepThreadUntil (pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter ();
	return !((GLOBAL (CurrentActivity) & CHECK_ABORT) || 
		 PulsedInputState.key[KEY_MENU_CANCEL] || done);
}

void
SetupMenu (void)
{
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = FALSE;
	SetMenuSounds (0, MENU_SOUND_SELECT);
	done = FALSE;
	DoInput ((PVOID)&s, TRUE);
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	PropagateResults ();
}

void
GetGlobalOptions (GLOBALOPTS *opts)
{
	if (GfxFlags & TFB_GFXFLAGS_SCALE_BILINEAR) 
	{
		opts->scaler = OPTVAL_BILINEAR_SCALE;
	}
	else if (GfxFlags & TFB_GFXFLAGS_SCALE_BIADAPT)
	{
		opts->scaler = OPTVAL_BIADAPT_SCALE;
	}
	else if (GfxFlags & TFB_GFXFLAGS_SCALE_BIADAPTADV) 
	{
		opts->scaler = OPTVAL_BIADV_SCALE;
	}
	else if (GfxFlags & TFB_GFXFLAGS_SCALE_TRISCAN) 
	{
		opts->scaler = OPTVAL_TRISCAN_SCALE;
	} 
	else
	{
		opts->scaler = OPTVAL_NO_SCALE;
	}
	opts->subtitles = optSubtitles ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->scanlines = (GfxFlags & TFB_GFXFLAGS_SCANLINES) ? 
		OPTVAL_ENABLED : OPTVAL_DISABLED;
	// opts->music = (optWhichMusic == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->menu = (optWhichMenu == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->text = (optWhichFonts == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->cscan = (optWhichCoarseScan == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->scroll = (optSmoothScroll == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;

	switch (ScreenColorDepth) 
	{
	case 16:
		opts->depth = OPTVAL_16;
		break;
	case 24:
		opts->depth = OPTVAL_24;
		break;
	case 32:
		opts->depth = OPTVAL_32;
		break;
	default:
		opts->depth = OPTVAL_32+1;
		break;
	}

	switch (ScreenWidthActual)
	{
	case 320:
		if (GraphicsDriver == TFB_GFXDRIVER_SDL_PURE)
		{
			opts->driver = OPTVAL_320_240_PURE;
		}
		else
		{
			if (ScreenHeightActual != 240)
			{
				opts->driver = OPTVAL_CUSTOM_GL;
			}
			else
			{
				opts->driver = OPTVAL_320_240_GL;
			}
		}
		break;
	case 640:
		if (GraphicsDriver == TFB_GFXDRIVER_SDL_PURE)
		{
			opts->driver = OPTVAL_640_480_PURE;
		}
		else
		{
			if (ScreenHeightActual != 480)
			{
				opts->driver = OPTVAL_CUSTOM_GL;
			}
			else
			{
				opts->driver = OPTVAL_640_480_GL;
			}
		}
		break;
	case 800:
		if (ScreenHeightActual != 600)
		{
			opts->driver = OPTVAL_CUSTOM_GL;
		}
		else
		{
			opts->driver = OPTVAL_800_600_GL;
		}
		break;
	case 1024:
		if (ScreenHeightActual != 768)
		{
			opts->driver = OPTVAL_CUSTOM_GL;
		}
		else
		{
			opts->driver = OPTVAL_1024_768_GL;
		}		
		break;
	default:
		opts->driver = OPTVAL_CUSTOM_GL;
		break;
	}
}

void
SetGlobalOptions (GLOBALOPTS *opts)
{
	int NewGfxFlags = GfxFlags;
	int NewWidth = ScreenWidthActual;
	int NewHeight = ScreenHeightActual;
	int NewDepth = ScreenColorDepth;
	int NewDriver = GraphicsDriver;

	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_BILINEAR;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_BIADAPT;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_BIADAPTADV;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_TRISCAN;

	switch (opts->driver) {
	case OPTVAL_320_240_PURE:
		NewWidth = 320;
		NewHeight = 240;
		NewDriver = TFB_GFXDRIVER_SDL_PURE;
		break;
	case OPTVAL_320_240_GL:
		NewWidth = 320;
		NewHeight = 240;
		NewDriver = TFB_GFXDRIVER_SDL_OPENGL;
		break;
	case OPTVAL_640_480_PURE:
		NewWidth = 640;
		NewHeight = 480;
		NewDriver = TFB_GFXDRIVER_SDL_PURE;
		break;
	case OPTVAL_640_480_GL:
		NewWidth = 640;
		NewHeight = 480;
		NewDriver = TFB_GFXDRIVER_SDL_OPENGL;
		break;
	case OPTVAL_800_600_GL:
		NewWidth = 800;
		NewHeight = 600;
		NewDriver = TFB_GFXDRIVER_SDL_OPENGL;
		break;
	case OPTVAL_1024_768_GL:
		NewWidth = 1024;
		NewHeight = 768;
		NewDriver = TFB_GFXDRIVER_SDL_OPENGL;
		break;
	default:
		/* Don't mess with the custom value */
		break;
	}

	switch (opts->depth) {
	case OPTVAL_16:
		NewDepth = 16;
		break;
	case OPTVAL_24:
		NewDepth = 24;
		break;
	case OPTVAL_32:
		NewDepth = 32;
		break;
	default:
		fprintf (stderr, "Can't Happen: Impossible value for BPP\n");
		break;
	}

	switch (opts->scaler) {
	case OPTVAL_BILINEAR_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_BILINEAR;
		break;
	case OPTVAL_BIADAPT_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPT;
		break;
	case OPTVAL_BIADV_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPTADV;
		break;
	case OPTVAL_TRISCAN_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_TRISCAN;
		break;
	default:
		/* OPTVAL_NO_SCALE has no equivalent in gfxflags. */
		break;
	}
	if (opts->scanlines) {
		NewGfxFlags |= TFB_GFXFLAGS_SCANLINES;
	} else {
		NewGfxFlags &= ~TFB_GFXFLAGS_SCANLINES;
	}

	if ((NewWidth != ScreenWidthActual) ||
	    (NewHeight != ScreenHeightActual) ||
	    (NewDepth != ScreenColorDepth) ||
	    (NewDriver != GraphicsDriver) ||
	    (NewGfxFlags != GfxFlags)) 
	{
		FlushGraphics ();
		TFB_DrawScreen_ReinitVideo (NewDriver, NewGfxFlags, NewWidth, NewHeight, NewDepth);
		FlushGraphics ();
	}
	optSubtitles = (opts->subtitles == OPTVAL_ENABLED) ? TRUE : FALSE;
	// optWhichMusic = (opts->music == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichMenu = (opts->menu == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichFonts = (opts->text == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichCoarseScan = (opts->cscan == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optSmoothScroll = (opts->scroll == OPTVAL_3DO) ? OPT_3DO : OPT_PC;

}
