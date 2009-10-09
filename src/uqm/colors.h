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

#if 0
#define DEFAULT_COLOR_00 \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define DEFAULT_COLOR_01 \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define DEFAULT_COLOR_02 \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02)
#define DEFAULT_COLOR_03 \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03)
#define DEFAULT_COLOR_04 \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define DEFAULT_COLOR_05 \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x14), 0x05)
#define DEFAULT_COLOR_06 \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x00), 0x06)
#define DEFAULT_COLOR_07 \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define DEFAULT_COLOR_08 \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define DEFAULT_COLOR_09 \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09)
#define DEFAULT_COLOR_0A \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x0A), 0x0A)
#define DEFAULT_COLOR_0B \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B)
#define DEFAULT_COLOR_0C \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)
#define DEFAULT_COLOR_0D \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x1F), 0x0D)
#define DEFAULT_COLOR_0E \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E)
#define DEFAULT_COLOR_0F \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#endif


#define BLACK_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)
#define LTGRAY_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define DKGRAY_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08)
#define VDKGRAY_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x06, 0x06, 0x06), 0x00)
#define WHITE_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)

#define NORMAL_ILLUMINATED_COLOR \
		WHITE_COLOR
#define NORMAL_SHADOWED_COLOR \
		DKGRAY_COLOR
#define HIGHLIGHT_ILLUMINATED_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)
#define HIGHLIGHT_SHADOWED_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define MENU_BACKGROUND_COLOR \
		LTGRAY_COLOR
#define MENU_FOREGROUND_COLOR \
		DKGRAY_COLOR
#define MENU_TEXT_COLOR \
		VDKGRAY_COLOR
#define MENU_HIGHLIGHT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E)
#define MENU_CURSOR_COLOR \
		WHITE_COLOR

#define STATUS_ILLUMINATED_COLOR \
		WHITE_COLOR
#define STATUS_SHADOWED_COLOR \
		DKGRAY_COLOR
#define STATUS_SHAPE_COLOR \
		BLACK_COLOR
#define STATUS_SHAPE_OUTLINE_COLOR \
		WHITE_COLOR

#define CONTROL_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)

#define ALLIANCE_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define HIERARCHY_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define ALLIANCE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B)
#define HIERARCHY_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)
#define ALLIANCE_BOX_HIGHLIGHT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define HIERARCHY_BOX_HIGHLIGHT_COLOR \
		HIERARCHY_BACKGROUND_COLOR

#define MESSAGE_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define MESSAGE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)


// Not highlighted dialog options in comm.
#define COMM_PLAYER_TEXT_NORMAL_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03)

// Currently highlighted dialog option in comm.
#define COMM_PLAYER_TEXT_HIGHLIGHT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1A, 0x1A, 0x1A), 0x12)

// Background color of the area containing the player's dialog options.
#define COMM_PLAYER_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)

// "(In response to your statement)"
#define COMM_RESPONSE_INTRO_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0C, 0x1F), 0x48)
		
// Your dialog option after choosing it.
#define COMM_FEEDBACK_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x12, 0x14, 0x4F), 0x44)

// The background when reviewing the conversation history.
#define COMM_HISTORY_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x05, 0x00), 0x00)

// The text when reviewing the conversation history.
#define COMM_HISTORY_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x6B)

// The text "MORE" when reviewing the conversation history.
#define COMM_MORE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x17, 0x00), 0x00)

// Default colors for System Dialog Boxes (DrawShadowedBox)
#define SHADOWBOX_BACKGROUND_COLOR \
		MENU_BACKGROUND_COLOR

#define SHADOWBOX_MEDIUM_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19)

#define SHADOWBOX_DARK_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F)

#endif  /* _COLORS_H */

