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
#include "libs/reslib.h"

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
static WIDGET *current, *next;

static int quit_main_menu (WIDGET *self, int event);
static int quit_sub_menu (WIDGET *self, int event);
static int do_graphics (WIDGET *self, int event);
static int do_engine (WIDGET *self, int event);
static int do_resources (WIDGET *self, int event);
static int do_keyconfig (WIDGET *self, int event);

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

static CHOICE_OPTION res_opts[] = {
	{ "320x240",
		{ "320x240 resolution.",
		  "Available with SDL framebuffer or OpenGL.",
		  ""
		} },
	{ "640x480",
		{ "640x480 resolution.",
		  "Available with SDL framebuffer or OpenGL.",
		  ""
		} },
#ifdef HAVE_OPENGL
	{ "800x600",
		{ "800x600 resolution.",
		  "Requires OpenGL graphics drivers.",
		  ""
		} },
	{ "1024x768",
		{ "1024x768 resolution.",
		  "Requires OpenGL graphics drivers.",
		  ""
		} },
	{ "Custom",
		{ "Custom resolution set from the commandline.",
		  "Requires OpenGL graphics drivers.",
		  ""
		} },
#endif
};

static CHOICE_OPTION driver_opts[] = {
	{ "If Possible",
		{ "Uses SDL Framebuffer mode if possible,",
		  "and OpenGL otherwise.  Framebuffer mode",
		  "is available for 320x240 and 640x480."
		} },
	{ "Never",
		{ "Always use OpenGL.",
		  "OpenGL can produce any resolution and is",
		  "usually hardware-accelerated."
		} },
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

static CHOICE_OPTION fullscreen_opts[] = {
	{ "Windowed",
		{ "Display everything in a window.",
		  "(if windowing is available)",
		  ""
		} },
	{ "Fullscreen",
		{ "Game occupies the entire screen.",
		  "(if available)",
		  ""
		} },
};


#ifdef HAVE_OPENGL
#define RES_OPTS 4
#else
#define RES_OPTS 2
#endif

static WIDGET_CHOICE cmdline_opts[] = {
	{ CHOICE_PREFACE, "Resolution", RES_OPTS, 3, res_opts, 0, 0},
	{ CHOICE_PREFACE, "Use Framebuffer?", 2, 2, driver_opts, 0, 0},
	{ CHOICE_PREFACE, "Color depth", 3, 3, bpp_opts, 0, 0 },
	{ CHOICE_PREFACE, "Scaler", 5, 3, scaler_opts, 0, 0 },
  	{ CHOICE_PREFACE, "Scanlines", 2, 3, scanlines_opts, 0, 0 },
	{ CHOICE_PREFACE, "Menu Style", 2, 2, menu_opts, 0, 0 },
	{ CHOICE_PREFACE, "Font Style", 2, 2, font_opts, 0, 0 },
	{ CHOICE_PREFACE, "Scan Style", 2, 2, scan_opts, 0, 0 },
	{ CHOICE_PREFACE, "Scroll Style", 2, 2, scroll_opts, 0, 0 },
	{ CHOICE_PREFACE, "Subtitles", 2, 2, subtitles_opts, 0, 0 },
	{ CHOICE_PREFACE, "Music Driver", 2, 3, music_opts, 0, 0 },
	{ CHOICE_PREFACE, "Display", 2, 2, fullscreen_opts, 0, 0} };

static WIDGET_BUTTON quit_button = BUTTON_INIT (quit_main_menu, 
		"Quit Setup Menu", 
		"Return to the main menu.", "", "");

static WIDGET_BUTTON prev_button = BUTTON_INIT (quit_sub_menu, 
		"Return to Main Menu",
		"Save changes and return to main menu.",
		"Changes will not be applied until",
		"you quit setup entirely.");

static WIDGET_BUTTON graphics_button = BUTTON_INIT (do_graphics, "Graphics Options",
	     "Configure display options for UQM.",
	     "Graphics drivers, resolution, and scalers.", "");

static WIDGET_BUTTON engine_button = BUTTON_INIT (do_engine, "PC/3do Compatibility Options",
	     "Configure behavior of UQM", 
	     "to more closely match the PC or 3do behavior.", "");

static WIDGET_BUTTON resources_button = BUTTON_INIT (do_resources, "Resources Options",
	     "Configure UQM Addon Packs.", "", "Currently unimplemented.");

static WIDGET_BUTTON keyconfig_button = BUTTON_INIT (do_keyconfig, "Configure Controls",
	     "Set up keyboard and joystick controls.", "", "Currently unimplemented.");

static const char *incomplete_msg[] = { 
	"This part of the configuration",
	"has not yet been implemented.",
	"",
	"Expect it in a future version." };

static WIDGET_LABEL incomplete_label = {
	LABEL_PREFACE,
	4, incomplete_msg };

static WIDGET *main_widgets[] = {
	(WIDGET *)(&graphics_button),
	(WIDGET *)(&engine_button),
	(WIDGET *)(&resources_button),
	(WIDGET *)(&keyconfig_button),
	(WIDGET *)(&quit_button) };

static WIDGET *graphics_widgets[] = {
	(WIDGET *)(&cmdline_opts[0]),
#ifdef HAVE_OPENGL
	(WIDGET *)(&cmdline_opts[1]),
#endif
	(WIDGET *)(&cmdline_opts[11]),
	(WIDGET *)(&cmdline_opts[2]),
	(WIDGET *)(&cmdline_opts[3]),
	(WIDGET *)(&cmdline_opts[4]),
	(WIDGET *)(&prev_button) };

static WIDGET *engine_widgets[] = {
	(WIDGET *)(&cmdline_opts[5]),
	(WIDGET *)(&cmdline_opts[6]),
	(WIDGET *)(&cmdline_opts[7]),
	(WIDGET *)(&cmdline_opts[8]),
	(WIDGET *)(&cmdline_opts[9]),
	(WIDGET *)(&prev_button) };

static WIDGET *incomplete_widgets[] = {
	(WIDGET *)(&incomplete_label),
	(WIDGET *)(&prev_button) };
	     
static WIDGET_MENU_SCREEN menu = {
	MENU_SCREEN_PREFACE,
	"Ur-Quan Masters Setup",
	"",
	{ {0, 0}, NULL },
	5, main_widgets,
	0 };

static WIDGET_MENU_SCREEN graphics_menu = {
	MENU_SCREEN_PREFACE,
	"Ur-Quan Masters Setup",
	"Graphics Options",
	{ {0, 0}, NULL },
#ifdef HAVE_OPENGL
	7, graphics_widgets,
#else
	6, graphics_widgets,
#endif
	0 };	

static WIDGET_MENU_SCREEN engine_menu = {
	MENU_SCREEN_PREFACE,
	"Ur-Quan Masters Setup",
	"3do/PC Options",
	{ {0, 0}, NULL },
	6, engine_widgets,
	0 };	

static WIDGET_MENU_SCREEN resources_menu = {
	MENU_SCREEN_PREFACE,
	"Ur-Quan Masters Setup",
	"Addon Packs",
	{ {0, 0}, NULL },
	2, incomplete_widgets,
	0 };

static WIDGET_MENU_SCREEN keyconfig_menu = {
	MENU_SCREEN_PREFACE,
	"Ur-Quan Masters Setup",
	"Controls Setup",
	{ {0, 0}, NULL },
	2, incomplete_widgets,
	0 };

static int
quit_main_menu (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = NULL;
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
quit_sub_menu (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menu);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_graphics (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&graphics_menu);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_engine (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&engine_menu);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_resources (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&resources_menu);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_keyconfig (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&keyconfig_menu);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (ONE_SECOND / 20)

static void
SetDefaults (void)
{
	GLOBALOPTS opts;
	
	GetGlobalOptions (&opts);
	if (opts.res == OPTVAL_CUSTOM)
	{
		cmdline_opts[0].numopts = RES_OPTS + 1;
	}
	else
	{
		cmdline_opts[0].numopts = RES_OPTS;
	}
	cmdline_opts[0].selected = opts.res;
	cmdline_opts[1].selected = opts.driver;
	cmdline_opts[2].selected = opts.depth;
	cmdline_opts[3].selected = opts.scaler;
	cmdline_opts[4].selected = opts.scanlines;
	cmdline_opts[5].selected = opts.menu;
	cmdline_opts[6].selected = opts.text;
	cmdline_opts[7].selected = opts.cscan;
	cmdline_opts[8].selected = opts.scroll;
	cmdline_opts[9].selected = opts.subtitles;
	cmdline_opts[11].selected = opts.fullscreen;
}

static void
PropagateResults (void)
{
	GLOBALOPTS opts;
	opts.res = cmdline_opts[0].selected;
	opts.driver = cmdline_opts[1].selected;
	opts.depth = cmdline_opts[2].selected;
	opts.scaler = cmdline_opts[3].selected;
	opts.scanlines = cmdline_opts[4].selected;
	opts.menu = cmdline_opts[5].selected;
	opts.text = cmdline_opts[6].selected;
	opts.cscan = cmdline_opts[7].selected;
	opts.scroll = cmdline_opts[8].selected;
	opts.subtitles = cmdline_opts[9].selected;
	opts.fullscreen = cmdline_opts[11].selected;

	SetGlobalOptions (&opts);
}

static BOOLEAN
DoSetupMenu (PSETUP_MENU_STATE pInputState)
{
	if (!pInputState->initialized) 
	{
		SetDefaultMenuRepeatDelay ();
		pInputState->NextTime = GetTimeCounter ();
		SetDefaults ();

		current = NULL;
		next = (WIDGET *)(&menu);
		
		pInputState->initialized = TRUE;
	}
	if (!menu.bgStamp.frame)
	{
		FRAME f = CaptureDrawable (LoadCelFile (MENU_BKG));
		menu.bgStamp.origin.x = 0;
		menu.bgStamp.origin.y = 0;
		menu.bgStamp.frame = SetAbsFrameIndex (f, 0);
		graphics_menu.bgStamp.origin.x = 0;
		graphics_menu.bgStamp.origin.y = 0;
		graphics_menu.bgStamp.frame = SetAbsFrameIndex (f, 1);
		engine_menu.bgStamp.origin.x = 0;
		engine_menu.bgStamp.origin.y = 0;
		engine_menu.bgStamp.frame = SetAbsFrameIndex (f, 2);
		resources_menu.bgStamp.origin.x = 0;
		resources_menu.bgStamp.origin.y = 0;
		resources_menu.bgStamp.frame = SetAbsFrameIndex (f, 3);
		keyconfig_menu.bgStamp.origin.x = 0;
		keyconfig_menu.bgStamp.origin.y = 0;
		keyconfig_menu.bgStamp.frame = SetAbsFrameIndex (f, 1);
	}
	if (current != next)
	{
		SetTransitionSource (NULL);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
	}
	
	BatchGraphics ();
	(*next->draw)(next, 0, 0);

	if (current != next)
	{
		ScreenTransition (3, NULL);
		current = next;
	}

	UnbatchGraphics ();

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

	SleepThreadUntil (pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter ();
	return !((GLOBAL (CurrentActivity) & CHECK_ABORT) || 
		 PulsedInputState.key[KEY_MENU_CANCEL] || (next == NULL));
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
	opts->fullscreen = (GfxFlags & TFB_GFXFLAGS_FULLSCREEN) ?
			OPTVAL_ENABLED : OPTVAL_DISABLED;
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
			opts->res = OPTVAL_320_240;
			opts->driver = OPTVAL_PURE_IF_POSSIBLE;
		}
		else
		{
			opts->driver = OPTVAL_ALWAYS_GL;
			if (ScreenHeightActual != 240)
			{
				opts->res = OPTVAL_CUSTOM;
			}
			else
			{
				opts->res = OPTVAL_320_240;
			}
		}
		break;
	case 640:
		if (GraphicsDriver == TFB_GFXDRIVER_SDL_PURE)
		{
			opts->res = OPTVAL_640_480;
			opts->driver = OPTVAL_PURE_IF_POSSIBLE;
		}
		else
		{
			opts->driver = OPTVAL_ALWAYS_GL;
			if (ScreenHeightActual != 480)
			{
				opts->res = OPTVAL_CUSTOM;
			}
			else
			{
				opts->res = OPTVAL_640_480;
			}
		}
		break;
	case 800:
		opts->driver = OPTVAL_ALWAYS_GL;
		if (ScreenHeightActual != 600)
		{
			opts->res = OPTVAL_CUSTOM;
		}
		else
		{
			opts->res = OPTVAL_800_600;
		}
		break;
	case 1024:
		opts->driver = OPTVAL_ALWAYS_GL;
		if (ScreenHeightActual != 768)
		{
			opts->res = OPTVAL_CUSTOM;
		}
		else
		{
			opts->res = OPTVAL_1024_768;
		}		
		break;
	default:
		opts->driver = OPTVAL_ALWAYS_GL;
		opts->res = OPTVAL_CUSTOM;
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

	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_ANY;

	switch (opts->res) {
	case OPTVAL_320_240:
		NewWidth = 320;
		NewHeight = 240;
#ifdef HAVE_OPENGL	       
		NewDriver = (opts->driver == OPTVAL_ALWAYS_GL ? TFB_GFXDRIVER_SDL_OPENGL : TFB_GFXDRIVER_SDL_PURE);
#else
		NewDriver = TFB_GFXDRIVER_SDL_PURE;
#endif
		break;
	case OPTVAL_640_480:
		NewWidth = 640;
		NewHeight = 480;
#ifdef HAVE_OPENGL	       
		NewDriver = (opts->driver == OPTVAL_ALWAYS_GL ? TFB_GFXDRIVER_SDL_OPENGL : TFB_GFXDRIVER_SDL_PURE);
#else
		NewDriver = TFB_GFXDRIVER_SDL_PURE;
#endif
		break;
	case OPTVAL_800_600:
		NewWidth = 800;
		NewHeight = 600;
		NewDriver = TFB_GFXDRIVER_SDL_OPENGL;
		break;
	case OPTVAL_1024_768:
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

	res_PutInteger ("config.reswidth", NewWidth);
	res_PutInteger ("config.resheight", NewHeight);
	res_PutInteger ("config.bpp", NewDepth);
	res_PutBoolean ("config.alwaysgl", opts->driver == OPTVAL_ALWAYS_GL);


	switch (opts->scaler) {
	case OPTVAL_BILINEAR_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_BILINEAR;
		res_PutString ("config.scaler", "bilinear");
		break;
	case OPTVAL_BIADAPT_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPT;
		res_PutString ("config.scaler", "biadapt");
		break;
	case OPTVAL_BIADV_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_BIADAPTADV;
		res_PutString ("config.scaler", "biadv");
		break;
	case OPTVAL_TRISCAN_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_TRISCAN;
		res_PutString ("config.scaler", "triscan");
		break;
	default:
		/* OPTVAL_NO_SCALE has no equivalent in gfxflags. */
		res_PutString ("config.scaler", "no");
		break;
	}
	if (opts->scanlines) {
		NewGfxFlags |= TFB_GFXFLAGS_SCANLINES;
	} else {
		NewGfxFlags &= ~TFB_GFXFLAGS_SCANLINES;
	}
	if (opts->fullscreen)
		NewGfxFlags |= TFB_GFXFLAGS_FULLSCREEN;
	else
		NewGfxFlags &= ~TFB_GFXFLAGS_FULLSCREEN;

	res_PutBoolean ("config.scanlines", opts->scanlines);
	res_PutBoolean ("config.fullscreen", opts->fullscreen);


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

	res_PutBoolean ("config.subtitles", opts->subtitles == OPTVAL_ENABLED);
	res_PutBoolean ("config.textmenu", opts->menu == OPTVAL_PC);
	res_PutBoolean ("config.textgradients", opts->text == OPTVAL_PC);
	res_PutBoolean ("config.iconicscan", opts->cscan == OPTVAL_3DO);
	res_PutBoolean ("config.smoothscroll", opts->scroll == OPTVAL_3DO);

	res_SaveFilename (configDir, "uqm.cfg", "config.");
}
