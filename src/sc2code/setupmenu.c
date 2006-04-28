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
#include "libs/strlib.h"
#include "libs/reslib.h"
#include "libs/sound/sound.h"
#include "libs/resource/stringbank.h"
#include "libs/log.h"
#include "resinst.h"
#include "nameref.h"

static STRING SetupTab;

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
static int do_audio (WIDGET *self, int event);
static int do_engine (WIDGET *self, int event);
static int do_resources (WIDGET *self, int event);
static int do_keyconfig (WIDGET *self, int event);
static int do_advanced (WIDGET *self, int event);

#ifdef HAVE_OPENGL
#define RES_OPTS 4
#else
#define RES_OPTS 2
#endif

#define MENU_COUNT    7
#define CHOICE_COUNT 20
#define SLIDER_COUNT  3
#define BUTTON_COUNT  8
#define LABEL_COUNT   2

/* The space for our widgets */
static WIDGET_MENU_SCREEN menus[MENU_COUNT];
static WIDGET_CHOICE choices[CHOICE_COUNT];
static WIDGET_SLIDER sliders[SLIDER_COUNT];
static WIDGET_BUTTON buttons[BUTTON_COUNT];
static WIDGET_LABEL labels[LABEL_COUNT];

/* The hardcoded data that isn't strings */

typedef int (*HANDLER)(WIDGET *, int);

static int choice_widths[CHOICE_COUNT] = {
	3, 2, 3, 3, 2, 2, 2, 2, 2,
	2, 2, 2, 3, 2, 2, 3, 3, 2,
	3, 3 };

static HANDLER button_handlers[BUTTON_COUNT] = {
	quit_main_menu, quit_sub_menu, do_graphics, do_engine,
	do_audio, do_resources, do_keyconfig, do_advanced };

static int menu_sizes[MENU_COUNT] = {
	7, 5, 6, 9, 2, 4,
#ifdef HAVE_OPENGL
	5 };
#else
	4 };
#endif

static int menu_bgs[MENU_COUNT] = { 0, 1, 1, 2, 3, 1, 2 };

/* These refer to uninitialized widgets, but that's OK; we'll fill
 * them in before we touch them */
static WIDGET *main_widgets[] = {
	(WIDGET *)(&buttons[2]),
	(WIDGET *)(&buttons[3]),
	(WIDGET *)(&buttons[4]),
	(WIDGET *)(&buttons[5]),
	(WIDGET *)(&buttons[6]),
	(WIDGET *)(&buttons[7]),
	(WIDGET *)(&buttons[0]) };

static WIDGET *graphics_widgets[] = {
	(WIDGET *)(&choices[0]),
	(WIDGET *)(&choices[10]),
	(WIDGET *)(&choices[2]),
	(WIDGET *)(&choices[3]),
	(WIDGET *)(&buttons[1]) };

static WIDGET *audio_widgets[] = {
	(WIDGET *)(&sliders[0]),
	(WIDGET *)(&sliders[1]),
	(WIDGET *)(&sliders[2]),
	(WIDGET *)(&choices[14]),
	(WIDGET *)(&choices[9]),
	(WIDGET *)(&buttons[1]) };

static WIDGET *engine_widgets[] = {
	(WIDGET *)(&choices[4]),
	(WIDGET *)(&choices[5]),
	(WIDGET *)(&choices[6]),
	(WIDGET *)(&choices[7]),
	(WIDGET *)(&choices[8]),
	(WIDGET *)(&choices[13]),
	(WIDGET *)(&choices[11]),
	(WIDGET *)(&choices[17]),
	(WIDGET *)(&buttons[1]) };

static WIDGET *advanced_widgets[] = {
#ifdef HAVE_OPENGL
	(WIDGET *)(&choices[1]),
#endif
	(WIDGET *)(&choices[12]),
	(WIDGET *)(&choices[15]),
	(WIDGET *)(&choices[16]),
	(WIDGET *)(&buttons[1]) };
	
static WIDGET *keyconfig_widgets[] = {
	(WIDGET *)(&choices[18]),
	(WIDGET *)(&choices[19]),
	(WIDGET *)(&labels[1]),
	(WIDGET *)(&buttons[1]) };

static WIDGET *incomplete_widgets[] = {
	(WIDGET *)(&labels[0]),
	(WIDGET *)(&buttons[1]) };

static WIDGET **menu_widgets[MENU_COUNT] = {
	main_widgets, graphics_widgets, audio_widgets, engine_widgets, 
	incomplete_widgets, keyconfig_widgets, advanced_widgets };

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
		next = (WIDGET *)(&menus[0]);
		(*next->receiveFocus) (next, WIDGET_EVENT_SELECT);
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
		next = (WIDGET *)(&menus[1]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_audio (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[2]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
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
		next = (WIDGET *)(&menus[3]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
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
		next = (WIDGET *)(&menus[4]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
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
		next = (WIDGET *)(&menus[5]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		return TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_advanced (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		next = (WIDGET *)(&menus[6]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
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
		choices[0].numopts = RES_OPTS + 1;
	}
	else
	{
		choices[0].numopts = RES_OPTS;
	}
	choices[0].selected = opts.res;
	choices[1].selected = opts.driver;
	choices[2].selected = opts.scaler;
	choices[3].selected = opts.scanlines;
	choices[4].selected = opts.menu;
	choices[5].selected = opts.text;
	choices[6].selected = opts.cscan;
	choices[7].selected = opts.scroll;
	choices[8].selected = opts.subtitles;
	choices[9].selected = opts.music;
	choices[10].selected = opts.fullscreen;
	choices[11].selected = opts.intro;
	choices[12].selected = opts.fps;
	choices[13].selected = opts.meleezoom;
	choices[14].selected = opts.stereo;
	choices[15].selected = opts.adriver;
	choices[16].selected = opts.aquality;
	choices[17].selected = opts.shield;
	choices[18].selected = opts.player1;
	choices[19].selected = opts.player2;

	sliders[0].value = opts.musicvol;
	sliders[1].value = opts.sfxvol;
	sliders[2].value = opts.speechvol;
}

static void
PropagateResults (void)
{
	GLOBALOPTS opts;
	opts.res = choices[0].selected;
	opts.driver = choices[1].selected;
	opts.scaler = choices[2].selected;
	opts.scanlines = choices[3].selected;
	opts.menu = choices[4].selected;
	opts.text = choices[5].selected;
	opts.cscan = choices[6].selected;
	opts.scroll = choices[7].selected;
	opts.subtitles = choices[8].selected;
	opts.music = choices[9].selected;
	opts.fullscreen = choices[10].selected;
	opts.intro = choices[11].selected;
	opts.fps = choices[12].selected;
	opts.meleezoom = choices[13].selected;
	opts.stereo = choices[14].selected;
	opts.adriver = choices[15].selected;
	opts.aquality = choices[16].selected;
	opts.shield = choices[17].selected;
	opts.player1 = choices[18].selected;
	opts.player2 = choices[19].selected;

	opts.musicvol = sliders[0].value;
	opts.sfxvol = sliders[1].value;
	opts.speechvol = sliders[2].value;
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
		next = (WIDGET *)(&menus[0]);
		(*next->receiveFocus) (next, WIDGET_EVENT_DOWN);
		
		pInputState->initialized = TRUE;
	}
	if (current != next)
	{
		SetTransitionSource (NULL);
	}
	
	BatchGraphics ();
	(*next->draw)(next, 0, 0);

	if (current != next)
	{
		ScreenTransition (3, NULL);
		current = next;
	}

	UnbatchGraphics ();

	if (PulsedInputState.menu[KEY_MENU_UP])
	{
		Widget_Event (WIDGET_EVENT_UP);
	}
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
	{
		Widget_Event (WIDGET_EVENT_DOWN);
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		Widget_Event (WIDGET_EVENT_LEFT);
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		Widget_Event (WIDGET_EVENT_RIGHT);
	}
	if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		Widget_Event (WIDGET_EVENT_SELECT);
	}
	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		Widget_Event (WIDGET_EVENT_CANCEL);
	}

	SleepThreadUntil (pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter ();
	return !((GLOBAL (CurrentActivity) & CHECK_ABORT) || 
		 (next == NULL));
}

static stringbank *bank = NULL;
static FRAME setup_frame = NULL;


static void
init_widgets (void)
{
	const char *buffer[100], *str, *title;
	int count, i, index;

	if (bank == NULL)
	{
		bank = StringBank_Create ();
	}
	
	if (setup_frame == NULL)
	{
		setup_frame = CaptureDrawable (LoadGraphic (MENUBKG_PMAP_ANIM));
	}

	count = GetStringTableCount (SetupTab);

	if (count < 3)
	{
		log_add (log_Always, "PANIC: Setup string table too short to even hold all indices!");
		exit (EXIT_FAILURE);
	}

	/* Menus */
	title = StringBank_AddOrFindString (bank, GetStringAddress (SetAbsStringTableIndex (SetupTab, 0)));
	if (SplitString (GetStringAddress (SetAbsStringTableIndex (SetupTab, 1)), '\n', 100, buffer, bank) != MENU_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Always, "PANIC: Incorrect number of Menu Subtitles");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < MENU_COUNT; i++)
	{
		menus[i].parent = NULL;
		menus[i].handleEvent = Widget_HandleEventMenuScreen;
		menus[i].receiveFocus = Widget_ReceiveFocusMenuScreen;
		menus[i].draw = Widget_DrawMenuScreen;
		menus[i].height = Widget_HeightFullScreen;
		menus[i].width = Widget_WidthFullScreen;
		menus[i].title = title;
		menus[i].subtitle = buffer[i];
		menus[i].bgStamp.origin.x = 0;
		menus[i].bgStamp.origin.y = 0;
		menus[i].bgStamp.frame = SetAbsFrameIndex (setup_frame, menu_bgs[i]);
		menus[i].num_children = menu_sizes[i];
		menus[i].child = menu_widgets[i];
		menus[i].highlighted = 0;
	}
		
	/* Options */
	if (SplitString (GetStringAddress (SetAbsStringTableIndex (SetupTab, 2)), '\n', 100, buffer, bank) != CHOICE_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Always, "PANIC: Incorrect number of Choice Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < CHOICE_COUNT; i++)
	{
		choices[i].parent = NULL;
		choices[i].handleEvent = Widget_HandleEventChoice;
		choices[i].receiveFocus = Widget_ReceiveFocusChoice;
		choices[i].draw = Widget_DrawChoice;
		choices[i].height = Widget_HeightChoice;
		choices[i].width = Widget_WidthFullScreen;
		choices[i].category = buffer[i];
		choices[i].numopts = 0;
		choices[i].options = NULL;
		choices[i].selected = 0;
		choices[i].highlighted = 0;
		choices[i].maxcolumns = choice_widths[i];
	}

	/* Fill in the options now */
	index = 3;  /* Index into string table */
	for (i = 0; i < CHOICE_COUNT; i++)
	{
		int j, optcount;

		if (index >= count)
		{
			log_add (log_Always, "PANIC: String table cut short while reading choices");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (SetAbsStringTableIndex (SetupTab, index++));
		optcount = SplitString (str, '\n', 100, buffer, bank);
		choices[i].numopts = optcount;
		choices[i].options = HMalloc (optcount * sizeof (CHOICE_OPTION));
		for (j = 0; j < optcount; j++)
		{
			choices[i].options[j].optname = buffer[j];
			choices[i].options[j].tooltip[0] = "";
			choices[i].options[j].tooltip[1] = "";
			choices[i].options[j].tooltip[2] = "";
		}
		for (j = 0; j < optcount; j++)
		{
			int k, tipcount;

			if (index >= count)
			{
				log_add (log_Always, "PANIC: String table cut short while reading choices");
				exit (EXIT_FAILURE);
			}
			str = GetStringAddress (SetAbsStringTableIndex (SetupTab, index++));
			tipcount = SplitString (str, '\n', 100, buffer, bank);			
			if (tipcount > 3)
			{
				tipcount = 3;
			}
			for (k = 0; k < tipcount; k++)
			{
				choices[i].options[j].tooltip[k] = buffer[k];
			}
		}
	}

	/* The first choice is resolution, and is handled specially */
	choices[0].numopts = RES_OPTS;

	/* Sliders */
	if (index >= count)
	{
		log_add (log_Always, "PANIC: String table cut short while reading sliders");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (SetupTab, index++)), '\n', 100, buffer, bank) != SLIDER_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Always, "PANIC: Incorrect number of Slider Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < SLIDER_COUNT; i++)
	{
		sliders[i].parent = NULL;
		sliders[i].handleEvent = Widget_HandleEventSlider;
		sliders[i].receiveFocus = Widget_ReceiveFocusSimple;
		sliders[i].draw = Widget_DrawSlider;
		sliders[i].height = Widget_HeightOneLine;
		sliders[i].width = Widget_WidthFullScreen;
		sliders[i].draw_value = Widget_Slider_DrawValue;
		sliders[i].min = 0;
		sliders[i].max = 100;
		sliders[i].step = 5;
		sliders[i].value = 75;
		sliders[i].category = buffer[i];
		sliders[i].tooltip[0] = "";
		sliders[i].tooltip[1] = "";
		sliders[i].tooltip[2] = "";
	}

	for (i = 0; i < SLIDER_COUNT; i++)
	{
		int j, tipcount;
		
		if (index >= count)
		{
			log_add (log_Always, "PANIC: String table cut short while reading sliders");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (SetAbsStringTableIndex (SetupTab, index++));
		tipcount = SplitString (str, '\n', 100, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			sliders[i].tooltip[j] = buffer[j];
		}
	}

	/* Buttons */
	if (index >= count)
	{
		log_add (log_Always, "PANIC: String table cut short while reading buttons");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (SetupTab, index++)), '\n', 100, buffer, bank) != BUTTON_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Always, "PANIC: Incorrect number of Button Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < BUTTON_COUNT; i++)
	{
		buttons[i].parent = NULL;
		buttons[i].handleEvent = button_handlers[i];
		buttons[i].receiveFocus = Widget_ReceiveFocusSimple;
		buttons[i].draw = Widget_DrawButton;
		buttons[i].height = Widget_HeightOneLine;
		buttons[i].width = Widget_WidthFullScreen;
		buttons[i].name = buffer[i];
		buttons[i].tooltip[0] = "";
		buttons[i].tooltip[1] = "";
		buttons[i].tooltip[2] = "";
	}

	for (i = 0; i < BUTTON_COUNT; i++)
	{
		int j, tipcount;
		
		if (index >= count)
		{
			log_add (log_Always, "PANIC: String table cut short while reading buttons");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (SetAbsStringTableIndex (SetupTab, index++));
		tipcount = SplitString (str, '\n', 100, buffer, bank);
		if (tipcount > 3)
		{
			tipcount = 3;
		}
		for (j = 0; j < tipcount; j++)
		{
			buttons[i].tooltip[j] = buffer[j];
		}
	}

	/* Labels */
	if (index >= count)
	{
		log_add (log_Always, "PANIC: String table cut short while reading labels");
		exit (EXIT_FAILURE);
	}

	if (SplitString (GetStringAddress (SetAbsStringTableIndex (SetupTab, index++)), '\n', 100, buffer, bank) != LABEL_COUNT)
	{
		/* TODO: Ignore extras instead of dying. */
		log_add (log_Always, "PANIC: Incorrect number of Label Options");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		labels[i].parent = NULL;
		labels[i].handleEvent = Widget_HandleEventIgnoreAll;
		labels[i].receiveFocus = Widget_ReceiveFocusRefuseFocus;
		labels[i].draw = Widget_DrawLabel;
		labels[i].height = Widget_HeightLabel;
		labels[i].width = Widget_WidthFullScreen;
		labels[i].line_count = 0;
		labels[i].lines = NULL;
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		int j, linecount;
		
		if (index >= count)
		{
			log_add (log_Always, "PANIC: String table cut short while reading labels");
			exit (EXIT_FAILURE);
		}
		str = GetStringAddress (SetAbsStringTableIndex (SetupTab, index++));
		linecount = SplitString (str, '\n', 100, buffer, bank);
		labels[i].line_count = linecount;
		labels[i].lines = (const char **)HMalloc(linecount * sizeof(const char *));
		for (j = 0; j < linecount; j++)
		{
			labels[i].lines[j] = buffer[j];
		}
	}

	/* Check for garbage at the end */
	if (index < count)
	{
		log_add (log_Warning, "WARNING: Setup strings had %d garbage entries at the end.",
				count - index);
	}
}

static void
clean_up_widgets (void)
{
	int i;

	for (i = 0; i < CHOICE_COUNT; i++)
	{
		if (choices[i].options)
		{
			HFree (choices[i].options);
		}
	}

	for (i = 0; i < LABEL_COUNT; i++)
	{
		if (labels[i].lines)
		{
			HFree (labels[i].lines);
		}
	}

	/* Clear out the master tables */
	
	if (SetupTab)
	{
		DestroyStringTable (ReleaseStringTable (SetupTab));
		SetupTab = 0;
	}
	if (bank)
	{
		StringBank_Free (bank);
		bank = NULL;
	}
	if (setup_frame)
	{
		DestroyDrawable (ReleaseDrawable (setup_frame));
		setup_frame = NULL;
	}
}

void
SetupMenu (void)
{
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = FALSE;
	SetMenuSounds (0, MENU_SOUND_SELECT);
	SetupTab = CaptureStringTable (LoadStringTable (SETUP_MENU_STRTAB));
	if (SetupTab) 
	{
		init_widgets ();
	}
	else
	{
		log_add (log_Always, "PANIC: Could not find strings for the setup menu!");
		exit (EXIT_FAILURE);
	}
	done = FALSE;

	DoInput ((PVOID)&s, TRUE);
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	PropagateResults ();
	if (SetupTab)
	{
		clean_up_widgets ();
	}
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
	else if (GfxFlags & TFB_GFXFLAGS_SCALE_HQXX)
	{
		opts->scaler = OPTVAL_HQXX_SCALE;
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
	opts->menu = (optWhichMenu == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->text = (optWhichFonts == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->cscan = (optWhichCoarseScan == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->scroll = (optSmoothScroll == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->intro = (optWhichIntro == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->shield = (optWhichShield == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	opts->fps = (GfxFlags & TFB_GFXFLAGS_SHOWFPS) ? 
			OPTVAL_ENABLED : OPTVAL_DISABLED;
	opts->meleezoom = (optMeleeScale == TFB_SCALE_TRILINEAR) ? 
			OPTVAL_3DO : OPTVAL_PC;
	opts->stereo = optStereoSFX ? OPTVAL_ENABLED : OPTVAL_DISABLED;
	/* These values are read in, but won't change during a run. */
	opts->music = (optWhichMusic == OPT_3DO) ? OPTVAL_3DO : OPTVAL_PC;
	switch (snddriver) {
	case audio_DRIVER_OPENAL:
		opts->adriver = OPTVAL_OPENAL;
		break;
	case audio_DRIVER_MIXSDL:
		opts->adriver = OPTVAL_MIXSDL;
		break;
	default:
		opts->adriver = OPTVAL_SILENCE;
		break;
	}
	if (soundflags & audio_QUALITY_HIGH)
	{
		opts->aquality = OPTVAL_HIGH;
	}
	else if (soundflags & audio_QUALITY_LOW)
	{
		opts->aquality = OPTVAL_LOW;
	}
	else
	{
		opts->aquality = OPTVAL_MEDIUM;
	}

	/* Work out resolution.  On the way, try to guess a good default
	 * for config.alwaysgl, then overwrite it if it was set previously. */
	opts->driver = OPTVAL_PURE_IF_POSSIBLE;
	switch (ScreenWidthActual)
	{
	case 320:
		if (GraphicsDriver == TFB_GFXDRIVER_SDL_PURE)
		{
			opts->res = OPTVAL_320_240;
		}
		else
		{
			if (ScreenHeightActual != 240)
			{
				opts->res = OPTVAL_CUSTOM;
			}
			else
			{
				opts->res = OPTVAL_320_240;
				opts->driver = OPTVAL_ALWAYS_GL;
			}
		}
		break;
	case 640:
		if (GraphicsDriver == TFB_GFXDRIVER_SDL_PURE)
		{
			opts->res = OPTVAL_640_480;
		}
		else
		{
			if (ScreenHeightActual != 480)
			{
				opts->res = OPTVAL_CUSTOM;
			}
			else
			{
				opts->res = OPTVAL_640_480;
				opts->driver = OPTVAL_ALWAYS_GL;
			}
		}
		break;
	case 800:
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
		opts->res = OPTVAL_CUSTOM;
		break;
	}

	if (res_HasKey ("config.alwaysgl"))
	{
		if (res_GetBoolean ("config.alwaysgl"))
		{
			opts->driver = OPTVAL_ALWAYS_GL;
		}
		else
		{
			opts->driver = OPTVAL_PURE_IF_POSSIBLE;
		}
	}

	opts->player1 = PlayerOne;
	opts->player2 = PlayerTwo;

	opts->musicvol = (((int)(musicVolumeScale * 100.0f) + 2) / 5) * 5;
	opts->sfxvol = (((int)(sfxVolumeScale * 100.0f) + 2) / 5) * 5;
	opts->speechvol = (((int)(speechVolumeScale * 100.0f) + 2) / 5) * 5;
	
}

void
SetGlobalOptions (GLOBALOPTS *opts)
{
	int NewGfxFlags = GfxFlags;
	int NewWidth = ScreenWidthActual;
	int NewHeight = ScreenHeightActual;
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

	res_PutInteger ("config.reswidth", NewWidth);
	res_PutInteger ("config.resheight", NewHeight);
	res_PutBoolean ("config.alwaysgl", opts->driver == OPTVAL_ALWAYS_GL);
	res_PutBoolean ("config.usegl", NewDriver == TFB_GFXDRIVER_SDL_OPENGL);

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
	case OPTVAL_HQXX_SCALE:
		NewGfxFlags |= TFB_GFXFLAGS_SCALE_HQXX;
		res_PutString ("config.scaler", "hq");
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
	    (NewDriver != GraphicsDriver) ||
	    (NewGfxFlags != GfxFlags)) 
	{
		FlushGraphics ();
		TFB_DrawScreen_ReinitVideo (NewDriver, NewGfxFlags, NewWidth, NewHeight);
		FlushGraphics ();
	}
	optSubtitles = (opts->subtitles == OPTVAL_ENABLED) ? TRUE : FALSE;
	// optWhichMusic = (opts->music == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichMenu = (opts->menu == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichFonts = (opts->text == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichCoarseScan = (opts->cscan == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optSmoothScroll = (opts->scroll == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichShield = (opts->shield == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optMeleeScale = (opts->meleezoom == OPTVAL_3DO) ? TFB_SCALE_TRILINEAR : TFB_SCALE_STEP;
	optWhichIntro = (opts->intro == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	PlayerOne = opts->player1;
	PlayerTwo = opts->player2;

	res_PutBoolean ("config.subtitles", opts->subtitles == OPTVAL_ENABLED);
	res_PutBoolean ("config.textmenu", opts->menu == OPTVAL_PC);
	res_PutBoolean ("config.textgradients", opts->text == OPTVAL_PC);
	res_PutBoolean ("config.iconicscan", opts->cscan == OPTVAL_3DO);
	res_PutBoolean ("config.smoothscroll", opts->scroll == OPTVAL_3DO);

	res_PutBoolean ("config.3domusic", opts->music == OPTVAL_3DO);
	res_PutBoolean ("config.3domovies", opts->intro == OPTVAL_3DO);
	res_PutBoolean ("config.showfps", opts->fps == OPTVAL_ENABLED);
	res_PutBoolean ("config.smoothmelee", opts->meleezoom == OPTVAL_3DO);
	res_PutBoolean ("config.positionalsfx", opts->stereo == OPTVAL_ENABLED); 
	res_PutBoolean ("config.pulseshield", opts->shield == OPTVAL_3DO);
	res_PutInteger ("config.player1control", opts->player1);
	res_PutInteger ("config.player2control", opts->player2);

	switch (opts->adriver) {
	case OPTVAL_SILENCE:
		res_PutString ("config.audiodriver", "none");
		break;
	case OPTVAL_MIXSDL:
		res_PutString ("config.audiodriver", "mixsdl");
		break;
	case OPTVAL_OPENAL:
		res_PutString ("config.audiodriver", "openal");
	default:
		/* Shouldn't happen; leave config untouched */
		break;
	}

	switch (opts->aquality) {
	case OPTVAL_LOW:
		res_PutString ("config.audioquality", "low");
		break;
	case OPTVAL_MEDIUM:
		res_PutString ("config.audioquality", "medium");
		break;
	case OPTVAL_HIGH:
		res_PutString ("config.audioquality", "high");
		break;
	default:
		/* Shouldn't happen; leave config untouched */
		break;
	}

	res_PutInteger ("config.musicvol", opts->musicvol);
	res_PutInteger ("config.sfxvol", opts->sfxvol);
	res_PutInteger ("config.speechvol", opts->speechvol);
	musicVolumeScale = opts->musicvol / 100.0f;
	sfxVolumeScale = opts->sfxvol / 100.0f;
	speechVolumeScale = opts->speechvol / 100.0f;
	// update actual volumes
	SetMusicVolume (musicVolume);
	SetSpeechVolume (speechVolumeScale);

	res_SaveFilename (configDir, "uqm.cfg", "config.");
}
