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

#ifndef _INPLIB_H
#define _INPLIB_H

#include "memlib.h"

typedef DWORD INPUT_STATE;

/* Serial Keyboard Pseudo UNICODE Equivalents */
#define SK_DELETE ((UNICODE)127)
#define SK_LF_ARROW ((UNICODE)128)
#define SK_RT_ARROW ((UNICODE)129)
#define SK_UP_ARROW ((UNICODE)130)
#define SK_DN_ARROW ((UNICODE)131)
#define SK_HOME ((UNICODE)132)
#define SK_PAGE_UP ((UNICODE)133)
#define SK_END ((UNICODE)134)
#define SK_PAGE_DOWN ((UNICODE)135)
#define SK_INSERT ((UNICODE)136)
#define SK_KEYPAD_PLUS ((UNICODE)137)
#define SK_KEYPAD_MINUS ((UNICODE)138)

#define SK_LF_SHIFT ((UNICODE)139)
#define SK_RT_SHIFT ((UNICODE)140)
#define SK_LF_CTL ((UNICODE)141)
#define SK_RT_CTL ((UNICODE)142)
#define SK_LF_ALT ((UNICODE)143)
#define SK_RT_ALT ((UNICODE)144)

#define SK_F1 ((UNICODE)145)
#define SK_F2 ((UNICODE)146)
#define SK_F3 ((UNICODE)147)
#define SK_F4 ((UNICODE)148)
#define SK_F5 ((UNICODE)149)
#define SK_F6 ((UNICODE)150)
#define SK_F7 ((UNICODE)151)
#define SK_F8 ((UNICODE)152)
#define SK_F9 ((UNICODE)153)
#define SK_F10 ((UNICODE)154)
#define SK_F11 ((UNICODE)155)
#define SK_F12 ((UNICODE)156)

extern BOOLEAN InitInput (void);
extern BOOLEAN UninitInput (void);
extern BOOLEAN AnyButtonPress (BOOLEAN DetectSpecial);

extern void FlushInput (void);
extern BOOLEAN KeyDown (UNICODE which_scan);
extern UNICODE GetUnicodeKey (void);
extern UNICODE KBDToUNICODE (UWORD which_key);

/*
 * Not used right now
extern BOOLEAN FindMouse (void);
extern void MoveMouse (SWORD x, SWORD y);
extern BYTE LocateMouse (PSWORD px, PSWORD py);
*/

/* Functions for dealing with Character Mode */

void EnableCharacterMode (void);
void DisableCharacterMode (void);
UNICODE GetCharacter (void);

#endif /* _INPLIB_H */


