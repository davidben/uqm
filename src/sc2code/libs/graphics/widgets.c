#include "graphics/gfx_common.h"
#include "graphics/widgets.h"

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
