#include "graphics/gfx_common.h"
#include "graphics/widgets.h"
#include "setup.h"
#include "units.h"

WIDGET *widget_focus = NULL;

void
DrawShadowedBox(PRECT r, COLOR bg, COLOR dark, COLOR medium)
{
	RECT t;
	COLOR oldcolor;

	BatchGraphics ();

	t.corner.x = r->corner.x - 2;
	t.corner.y = r->corner.y - 2;
	t.extent.width  = r->extent.width + 4;
	t.extent.height  = r->extent.height + 4;
	oldcolor = SetContextForeGroundColor (dark);
	DrawFilledRectangle (&t);

	t.corner.x += 2;
	t.corner.y += 2;
	t.extent.width -= 2;
	t.extent.height -= 2;
	SetContextForeGroundColor (medium);
	DrawFilledRectangle (&t);

	t.corner.x -= 1;
	t.corner.y += r->extent.height + 1;
	t.extent.height = 1;
	DrawFilledRectangle (&t);

	t.corner.x += r->extent.width + 2;
	t.corner.y -= r->extent.height + 2;
	t.extent.width = 1;
	DrawFilledRectangle (&t);

	SetContextForeGroundColor (bg);
	DrawFilledRectangle (r);

	SetContextForeGroundColor (oldcolor);
	UnbatchGraphics ();
}

static void
Widget_DrawToolTips (int numlines, const char **tips)
{
	RECT r;
	FONT  oldfont = SetContextFont (StarConFont);
	FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
	COLOR oldtext = SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
	TEXT t;
	int i;

	r.corner.x = 2;
	r.corner.y = 2;
	r.extent.width = SCREEN_WIDTH - 4;
	r.extent.height = SCREEN_HEIGHT - 4;

	t.align = ALIGN_CENTER;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = ~0;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + (r.extent.height - 8 - 8 * numlines);

	for (i = 0; i < numlines; i++)
	{
		t.pStr = tips[i];
		font_DrawText(&t);
		t.baseline.y += 8;
	}

	SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawMenuScreen (WIDGET *_self, int x, int y)
{
	RECT r;
	COLOR title, oldtext;
	COLOR inactive, default_color, selected;
	FONT  oldfont = SetContextFont (StarConFont);
	FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
	TEXT t;
	int widget_index, height, widget_y;

	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;
	
	r.corner.x = 2;
	r.corner.y = 2;
	r.extent.width = SCREEN_WIDTH - 4;
	r.extent.height = SCREEN_HEIGHT - 4;
	
	title = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);
	selected = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E);
	inactive = BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x1F), 0x07);
	default_color = title;
	
	DrawStamp (&self->bgStamp);
	
	oldtext = SetContextForeGroundColor (title);
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + 8;
	t.pStr = self->title;
	t.align = ALIGN_CENTER;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = ~0;
	font_DrawText (&t);
	t.baseline.y += 8;
	t.pStr = self->subtitle;
	font_DrawText (&t);

	height = 0;
	for (widget_index = 0; widget_index < self->num_children; widget_index++)
	{
		WIDGET *child = self->child[widget_index];
		height += (*child->height)(child);
		height += 8;  /* spacing */
	}

	height -= 8;

	widget_y = (SCREEN_HEIGHT - height) >> 1;
	for (widget_index = 0; widget_index < self->num_children; widget_index++)
	{
		WIDGET *c = self->child[widget_index];
		(*c->draw)(c, 0, widget_y);
		widget_y += (*c->height)(c) + 8;
	}
	
	SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);

	(void) x;
	(void) y;
}

void
Widget_DrawChoice (WIDGET *_self, int x, int y)
{
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	COLOR oldtext;
	COLOR inactive, default_color, selected;
	FONT  oldfont = SetContextFont (StarConFont);
	FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
	TEXT t;
	int i, home_x, home_y;
	
	default_color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);
	selected = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E);
	inactive = BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x1F), 0x07);

	t.baseline.x = x;
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (default_color);
	}
	font_DrawText (&t);

	home_x = t.baseline.x + 3 * (SCREEN_WIDTH / 8);
	home_y = t.baseline.y;
	t.align = ALIGN_CENTER;
	for (i = 0; i < self->numopts; i++)
	{
		t.baseline.x = home_x + ((i % 3) * (SCREEN_WIDTH / 4));
		t.baseline.y = home_y + (8 * (i / 3));
		t.pStr = self->options[i].optname;
		if ((widget_focus == _self) &&
		    (self->highlighted == i))
		{
			SetContextForeGroundColor (selected);
			Widget_DrawToolTips (3, self->options[i].tooltip);
		} 
		else if (i == self->selected)
		{
			SetContextForeGroundColor (default_color);
		}
		else
		{
			SetContextForeGroundColor (inactive);
		}
		font_DrawText (&t);
	}
	SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawButton (WIDGET *_self, int x, int y)
{
	WIDGET_BUTTON *self = (WIDGET_BUTTON *)_self;
	COLOR oldtext;
	COLOR inactive, selected;
	FONT  oldfont = SetContextFont (StarConFont);
	FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
	TEXT t;
	
	selected = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E);
	inactive = BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x1F), 0x07);

	t.baseline.x = 160;
	t.baseline.y = y;
	t.align = ALIGN_CENTER;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = ~0;
	t.pStr = self->name;
	if (widget_focus == _self)
	{
		Widget_DrawToolTips (3, self->tooltip);		
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (inactive);
	}
	font_DrawText (&t);
	SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
	(void) x;
}

int
Widget_HeightChoice (WIDGET *_self)
{
	return ((((WIDGET_CHOICE *)_self)->numopts + 2) / 3) * 8;
}

int
Widget_HeightFullScreen (WIDGET *_self)
{
	(void)_self;
	return SCREEN_HEIGHT;
}

int
Widget_HeightOneLine (WIDGET *_self)
{
	(void)_self;
	return 8;
}

int Widget_WidthFullScreen (WIDGET *_self)
{
	(void)_self;
	return SCREEN_WIDTH;
}

int
Widget_ReceiveFocusButton (WIDGET *_self, int event)
{
	widget_focus = _self;
	(void)event;
	return TRUE;
}

int
Widget_ReceiveFocusChoice (WIDGET *_self, int event)
{
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	widget_focus = _self;
	self->highlighted = self->selected;
	(void)event;
	return TRUE;
}

int
Widget_ReceiveFocusMenuScreen (WIDGET *_self, int event)
{
	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;
	int x, last_x, dx;
	for (x = 0; x < self->num_children; x++)
	{
		self->child[x]->parent = _self;
	}
	if (event == WIDGET_EVENT_UP)
	{
		x = self->num_children - 1;
		dx = -1;
		last_x = -1;
	}
	else
	{
		x = 0;
		dx = 1;
		last_x = self->num_children;
	}
	for ( ; x != last_x; x += dx) 
	{
		WIDGET *child = self->child[x];
		if ((*child->receiveFocus)(child, event))
		{
			self->highlighted = x;
			return TRUE;
		}
	}
	return FALSE;
}

int
Widget_HandleEventIgnoreAll (WIDGET *self, int event)
{
	(void)event;
	(void)self;
	return FALSE;
}

int
Widget_HandleEventChoice (WIDGET *_self, int event)
{
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	switch (event)
	{
	case WIDGET_EVENT_LEFT:
		self->highlighted -= 1;
		if (self->highlighted < 0)
			self->highlighted = self->numopts - 1;
		return TRUE;
	case WIDGET_EVENT_RIGHT:
		self->highlighted += 1;
		if (self->highlighted >= self->numopts)
			self->highlighted = 0;
		return TRUE;
	case WIDGET_EVENT_SELECT:
		self->selected = self->highlighted;
		return TRUE;
	default:
		return FALSE;
	}
}


int
Widget_HandleEventMenuScreen (WIDGET *_self, int event)
{
	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;
	int x, last_x, dx;
	switch (event)
	{
	case WIDGET_EVENT_UP:
		dx = -1;
		break;
	case WIDGET_EVENT_DOWN:
		dx = 1;
		break;
	default:
		return FALSE;
	}
	last_x = self->highlighted;
	x = self->highlighted + dx;
	while (x != last_x)
	{
		WIDGET *child;
		if (x == -1)
			x = self->num_children - 1;
		if (x == self->num_children)
			x = 0;
		child = self->child[x];
		if ((*child->receiveFocus)(child, event))
		{
			self->highlighted = x;
			return TRUE;
		}
		x += dx;
	}
	return FALSE;
}

int
Widget_Event (int event)
{
	WIDGET *widget = widget_focus;
	while (widget != NULL)
	{
		if ((*widget->handleEvent)(widget, event))
			return TRUE;
		widget = widget->parent;
	}
	return FALSE;
}
