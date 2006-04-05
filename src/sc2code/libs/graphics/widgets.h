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

#ifndef _WIDGETS_H
#define _WIDGETS_H

#include "libs/gfxlib.h"

enum {
	WIDGET_EVENT_UP,
	WIDGET_EVENT_DOWN,
	WIDGET_EVENT_LEFT,
	WIDGET_EVENT_RIGHT,
	WIDGET_EVENT_SELECT,
	WIDGET_EVENT_CANCEL,
	NUM_WIDGET_EVENTS
};

typedef struct _widget {
	struct _widget *parent;
	int (*handleEvent)(struct _widget *self, int event);
	int (*receiveFocus)(struct _widget *self, int event);
	void (*draw)(struct _widget *self, int x, int y);
	int (*height)(struct _widget *self);
	int (*width)(struct _widget *self);
} WIDGET;

typedef struct _widget_menu_screen {
	struct _widget *parent;
	int (*handleEvent)(struct _widget *self, int event);
	int (*receiveFocus)(struct _widget *self, int event);
	void (*draw)(struct _widget *self, int x, int y);
	int (*height)(struct _widget *self);
	int (*width)(struct _widget *self);
	const char *title;
	const char *subtitle;
	STAMP bgStamp;
	int num_children;
	struct _widget **child;
	int highlighted;
} WIDGET_MENU_SCREEN;

typedef struct {
	const char *optname;
	const char *tooltip[3];
} CHOICE_OPTION;

typedef struct _widget_choice {
	struct _widget *parent;
	int (*handleEvent)(struct _widget *self, int event);
	int (*receiveFocus)(struct _widget *self, int event);
	void (*draw)(struct _widget *self, int x, int y);
	int (*height)(struct _widget *self);
	int (*width)(struct _widget *self);
	const char *category;
	int numopts;
	int maxcolumns;
	CHOICE_OPTION *options;
	int selected, highlighted;
} WIDGET_CHOICE;

typedef struct _widget_button {
	struct _widget *parent;
	int (*handleEvent)(struct _widget *self, int event);
	int (*receiveFocus)(struct _widget *self, int event);
	void (*draw)(struct _widget *self, int x, int y);
	int (*height)(struct _widget *self);
	int (*width)(struct _widget *self);
	const char *name;
	const char *tooltip[3];
} WIDGET_BUTTON;

typedef struct _widget_label {
	struct _widget *parent;
	int (*handleEvent)(struct _widget *self, int event);
	int (*receiveFocus)(struct _widget *self, int event);
	void (*draw)(struct _widget *self, int x, int y);
	int (*height)(struct _widget *self);
	int (*width)(struct _widget *self);
	int line_count;
	const char **lines;
} WIDGET_LABEL;

typedef struct _widget_slider {
	struct _widget *parent;
	int (*handleEvent)(struct _widget *self, int event);
	int (*receiveFocus)(struct _widget *self, int event);
	void (*draw)(struct _widget *self, int x, int y);
	int (*height)(struct _widget *self);
	int (*width)(struct _widget *self);
	void (*draw_value)(struct _widget_slider *self, int x, int y);
	int min, max, step;
	int value;
	const char *category;
	const char *tooltip[3];
} WIDGET_SLIDER;

void DrawShadowedBox (PRECT r, COLOR bg, COLOR dark, COLOR medium);

int Widget_Event (int event);

/* Methods for filling in widgets with */

int Widget_ReceiveFocusMenuScreen (WIDGET *_self, int event);
int Widget_ReceiveFocusChoice (WIDGET *_self, int event);
int Widget_ReceiveFocusSimple (WIDGET *_self, int event);
int Widget_ReceiveFocusSlider (WIDGET *_self, int event);
int Widget_ReceiveFocusRefuseFocus (WIDGET *_self, int event);

int Widget_HandleEventMenuScreen (WIDGET *_self, int event);
int Widget_HandleEventChoice (WIDGET *_self, int event);
int Widget_HandleEventSlider (WIDGET *_self, int event);
int Widget_HandleEventIgnoreAll (WIDGET *_self, int event);

int Widget_HeightChoice (WIDGET *_self);
int Widget_HeightFullScreen (WIDGET *_self);
int Widget_HeightOneLine (WIDGET *_self);
int Widget_HeightLabel (WIDGET *_self);

int Widget_WidthFullScreen (WIDGET *_self);

void Widget_DrawMenuScreen (WIDGET *_self, int x, int y);
void Widget_DrawChoice (WIDGET *_self, int x, int y);
void Widget_DrawButton (WIDGET *_self, int x, int y);
void Widget_DrawLabel (WIDGET *_self, int x, int y);
void Widget_DrawSlider (WIDGET *_self, int x, int y);

void Widget_Slider_DrawValue (WIDGET_SLIDER *self, int x, int y);

#endif /* _WIDGETS_H */
