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
#ifndef _COLORS_H
#define _COLORS_H


#define BLACK_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define LTGRAY_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define DKGRAY_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define WHITE_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)

#define NORMAL_ILLUMINATED_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define NORMAL_SHADOWED_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define HIGHLIGHT_ILLUMINATED_COLOR    BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)
#define HIGHLIGHT_SHADOWED_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define MENU_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define MENU_FOREGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define MENU_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define MENU_HIGHLIGHT_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)

#define STATUS_ILLUMINATED_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define STATUS_SHADOWED_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define STATUS_SHAPE_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define STATUS_SHAPE_OUTLINE_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)

#define CONTROL_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)

#define ALLIANCE_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define HIERARCHY_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define ALLIANCE_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B)
#define HIERARCHY_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)
#define ALLIANCE_BOX_HIGHLIGHT_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define HIERARCHY_BOX_HIGHLIGHT_COLOR HIERARCHY_BACKGROUND_COLOR

#define MESSAGE_BACKGROUND_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define MESSAGE_TEXT_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)


#endif  /* _COLORS_H */

