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

#include "starcon.h"
#include "setupmenu.h"
#include "controls.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/widgets.h"

typedef struct setup_menu_state {
	BOOLEAN (*InputFunc) (struct setup_menu_state *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN initialized;
	int anim_frame_count;
	DWORD NextTime;
	int category;
	int option;
} SETUP_MENU_STATE;

typedef SETUP_MENU_STATE *PSETUP_MENU_STATE;

typedef struct {
	char *optname;
	char *tooltip[3];
} OPTION;

typedef struct {
	char *category;
	int numopts;
	OPTION * options;
	int selected;
} MENU_CATEGORY;

static OPTION scaler_opts[] = {
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

static OPTION scanlines_opts[] = {
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

static OPTION music_opts[] = {
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

static OPTION menu_opts[] = {
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

static OPTION font_opts[] = {
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

static OPTION scan_opts[] = {
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

static OPTION scroll_opts[] = {
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

static OPTION subtitles_opts[] = {
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

//static char *title_str = "Ur-Quan Masters Setup";
//static char *subtitle_str = "Graphics Options";
static char *title_str = "Use arrows and fire to select options";
static char *subtitle_str = "Press cancel when done";
static char **tooltip;

#define NUM_OPTS 6
static MENU_CATEGORY cmdline_opts[] = {
	// { "Scaler", 5, scaler_opts, 0 },
  	{ "Scanlines", 2, scanlines_opts, 0 },
	// { "Music Style", 2, music_opts, 0 },
	{ "Menu Style", 2, menu_opts, 0 },
	{ "Font Style", 2, font_opts, 0 },
	{ "Scan Style", 2, scan_opts, 0 },
	{ "Scroll Style", 2, scroll_opts, 0 },
	{ "Subtitles", 2, subtitles_opts, 0 },
	{ NULL, 0, NULL, 0 } };

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (ONE_SECOND / 60)

static void
SetDefaults (void)
{
	GLOBALOPTS opts;
	
	GetGlobalOptions (&opts);
	/*
	cmdline_opts[0].selected = opts.scaler;
	cmdline_opts[1].selected = opts.scanlines;
	cmdline_opts[2].selected = opts.music;
	cmdline_opts[3].selected = opts.menu;
	cmdline_opts[4].selected = opts.text;
	cmdline_opts[5].selected = opts.cscan;
	cmdline_opts[6].selected = opts.scroll;
	cmdline_opts[7].selected = opts.subtitles;
	*/
	cmdline_opts[0].selected = opts.scanlines;
	cmdline_opts[1].selected = opts.menu;
	cmdline_opts[2].selected = opts.text;
	cmdline_opts[3].selected = opts.cscan;
	cmdline_opts[4].selected = opts.scroll;
	cmdline_opts[5].selected = opts.subtitles;
}

static void
PropagateResults (void)
{
	GLOBALOPTS opts;
	/*
	opts.scaler = cmdline_opts[0].selected;
	opts.scanlines = cmdline_opts[1].selected;
	opts.music = cmdline_opts[2].selected;
	opts.menu = cmdline_opts[3].selected;
	opts.text = cmdline_opts[4].selected;
	opts.cscan = cmdline_opts[5].selected;
	opts.scroll = cmdline_opts[6].selected;
	opts.subtitles = cmdline_opts[7].selected;
	*/
	opts.scanlines = cmdline_opts[0].selected;
	opts.menu = cmdline_opts[1].selected;
	opts.text = cmdline_opts[2].selected;
	opts.cscan = cmdline_opts[3].selected;
	opts.scroll = cmdline_opts[4].selected;
	opts.subtitles = cmdline_opts[5].selected;

	SetGlobalOptions (&opts);
}

static void
DrawMenu (PSETUP_MENU_STATE pInputState)
{
	RECT r;
	COLOR bg, dark, medium, title, oldtext;
	COLOR inactive, default_color, selected;
	FONT  oldfont = SetContextFont (StarConFont);
	FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
	TEXT t;
	MENU_CATEGORY *menu;
	int cat_index;
	
	r.corner.x = 2;
	r.corner.y = 2;
	r.extent.width = SCREEN_WIDTH - 4;
	r.extent.height = SCREEN_HEIGHT - 4;
	
	dark = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x08), 0x01);
	medium = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x01);
	bg = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x1A), 0x09);

	title = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);
	selected = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E);
	inactive = BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x18), 0x07);
	default_color = title;
	
	DrawShadowedBox(&r, bg, dark, medium);
	
	oldtext = SetContextForeGroundColor (title);
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + 8;
	t.pStr = title_str;
	t.align = ALIGN_CENTER;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = ~0;
	font_DrawText (&t);
	t.baseline.y += 8;
	t.pStr = subtitle_str;
	font_DrawText (&t);

	t.baseline.y += 8;
	menu = cmdline_opts;
	cat_index = 0;

	while (menu->category)
	{
		int i, home_x, home_y;

		t.baseline.x = r.corner.x + (r.extent.width / 8);
		t.baseline.y += 16;
		t.pStr = menu->category;
		if (pInputState->category == cat_index)
		{
			SetContextForeGroundColor (selected);
		}
		else
		{
			SetContextForeGroundColor (inactive);
		}
		font_DrawText (&t);

		home_x = t.baseline.x + (r.extent.width / 4);
		home_y = t.baseline.y;
		for (i = 0; i < menu->numopts; i++)
		{
			t.baseline.x = home_x + ((i % 3) * (r.extent.width / 4));
			t.baseline.y = home_y + (8 * (i / 3));
			t.pStr = menu->options[i].optname;
			if ((pInputState->category == cat_index) &&
			    (pInputState->option == i))
			{
				SetContextForeGroundColor (selected);
				tooltip = menu->options[i].tooltip;
			} 
			else if (i == menu->selected)
			{
				SetContextForeGroundColor (default_color);
			}
			else
			{
				SetContextForeGroundColor (inactive);
			}
			font_DrawText (&t);
		}
		menu++;
		cat_index++;
	}
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + (r.extent.height - 32);
	t.pStr = tooltip[0];
	SetContextForeGroundColor (title);
	font_DrawText(&t);
	t.baseline.y += 8;
	t.pStr = tooltip[1];
	font_DrawText(&t);
	t.baseline.y += 8;
	t.pStr = tooltip[2];
	font_DrawText(&t);

	SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

BOOLEAN
DoSetupMenu (PSETUP_MENU_STATE pInputState)
{
	if (!pInputState->initialized) 
	{
		SetDefaultMenuRepeatDelay ();
		pInputState->anim_frame_count = NUM_STEPS;
		pInputState->initialized = TRUE;
		pInputState->NextTime = GetTimeCounter ();
		pInputState->category = 0;
		pInputState->option = 0;
		SetDefaults ();
	}
	
	BatchGraphics ();
	if (pInputState->anim_frame_count)
	{
		RECT r;
		COLOR bg, dark, medium;
		int frame = --pInputState->anim_frame_count;
		
		r.corner.x = ((frame * X_STEP) >> 1) + 2;
		r.corner.y = ((frame * Y_STEP) >> 1) + 2;
		r.extent.width = X_STEP * (NUM_STEPS - frame) - 4;
		r.extent.height = Y_STEP * (NUM_STEPS - frame) - 4;

		dark = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x08), 0x01);
		medium = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x01);
		bg = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x1A), 0x09);

		DrawShadowedBox (&r, bg, dark, medium);
	}
	else
	{
		int opt, cat;
		MENU_CATEGORY *menu = cmdline_opts;
		DrawMenu (pInputState);

		cat = pInputState->category;
		opt = pInputState->option;

		if (PulsedInputState.key[KEY_MENU_UP])
		{
			opt -= 3;
			if (opt < 0)
			{
				cat--;
				if (cat < 0)
				{
					cat = NUM_OPTS-1;
				}
				/* Preserve column if possible */
				opt = (menu[cat].numopts - (menu[cat].numopts % 3)) + ((opt + 3) % 3);
				if (opt >= menu[cat].numopts)
				{
					opt = menu[cat].numopts - 1;
				}
			}
		}
		else if (PulsedInputState.key[KEY_MENU_DOWN])
		{
			opt += 3;
			if ((opt - (opt % 3)) >= menu[cat].numopts)
			{
				cat++;
				if (cat >= NUM_OPTS)
				{
					cat = 0;
				}
				/* Preserve column if possible */
				opt = opt % 3;
				if (opt >= menu[cat].numopts)
				{
					opt = menu[cat].numopts - 1;
				}
			}
			else if (opt >= menu[cat].numopts)
			{
				opt = menu[cat].numopts - 1;
			}
		}
		else if (PulsedInputState.key[KEY_MENU_LEFT])
		{
			opt--;
			if (opt < 0)
			{
				cat--;
				if (cat < 0)
				{
					cat = NUM_OPTS-1;
				}
				opt = menu[cat].numopts - 1;
			}	
		}
		else if (PulsedInputState.key[KEY_MENU_RIGHT])
		{
			opt++;
			if (opt >= menu[cat].numopts)
			{
				cat++;
				if (cat >= NUM_OPTS)
				{
					cat = 0;
				}
				opt = 0;
			}	
		}
		pInputState->category = cat;
		pInputState->option = opt;
		if (PulsedInputState.key[KEY_MENU_SELECT])
		{
			cmdline_opts[cat].selected = opt;
		}
	}

	UnbatchGraphics ();
	SleepThreadUntil (pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter ();
	return !((GLOBAL (CurrentActivity) & CHECK_ABORT) || 
		 PulsedInputState.key[KEY_MENU_CANCEL]);
}

void
SetupMenu (void)
{
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = FALSE;
	SetMenuSounds (0, MENU_SOUND_SELECT);
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
}

void
SetGlobalOptions (GLOBALOPTS *opts)
{
	int NewGfxFlags = GfxFlags;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_BILINEAR;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_BIADAPT;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_BIADAPTADV;
	NewGfxFlags &= ~TFB_GFXFLAGS_SCALE_TRISCAN;

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

	GfxFlags = NewGfxFlags;
	optSubtitles = (opts->subtitles == OPTVAL_ENABLED) ? TRUE : FALSE;
	// optWhichMusic = (opts->music == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichMenu = (opts->menu == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichFonts = (opts->text == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optWhichCoarseScan = (opts->cscan == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
	optSmoothScroll = (opts->scroll == OPTVAL_3DO) ? OPT_3DO : OPT_PC;
}
