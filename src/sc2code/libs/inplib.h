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

typedef PVOID INPUT_REF;
typedef MEM_HANDLE INPUT_DEVICE;
typedef DWORD INPUT_STATE;

typedef enum
{
	KEYBOARD_DEVICE = 0,
	JOYSTICK_DEVICE,
	MOUSE_DEVICE
} INPUT_DEVTYPE;

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
#define SK_CTL ((UNICODE)141)
#define SK_ALT ((UNICODE)142)

#define KEY_DOWN ((BYTE)(1 << 0))
#define KEY_UP ((BYTE)(1 << 1))
#define LF_SHIFT_DOWN ((BYTE)(1 << 2))
#define RT_SHIFT_DOWN ((BYTE)(1 << 3))
#define CTL_DOWN ((BYTE)(1 << 4))
#define ALT_DOWN ((BYTE)(1 << 5))

#define SK_F1 ((UNICODE)143)
#define SK_F2 ((UNICODE)144)
#define SK_F3 ((UNICODE)145)
#define SK_F4 ((UNICODE)146)
#define SK_F5 ((UNICODE)147)
#define SK_F6 ((UNICODE)148)
#define SK_F7 ((UNICODE)149)
#define SK_F8 ((UNICODE)150)
#define SK_F9 ((UNICODE)151)
#define SK_F10 ((UNICODE)152)
#define SK_F11 ((UNICODE)153)
#define SK_F12 ((UNICODE)154)

#define DEVTYPE_MASK 0x03
#define GetInputUNICODE(is) (LOWORD (is))
#define GetInputXComponent(is) ((SBYTE)(LOBYTE (LOWORD (is))))
#define GetInputYComponent(is) ((SBYTE)(HIBYTE (LOWORD (is))))
#define GetInputDevType(is) ((INPUT_DEVTYPE)(HIWORD (is) & DEVTYPE_MASK))
#define SetInputUNICODE(is,a) \
		(*(is) = (*(is) & ~0x0000FFFFL) | (UNICODE)(a))
#define SetInputXComponent(is,x) \
		(*(is) = (*(is) & ~0x000000FFL) | (BYTE)(x))
#define SetInputYComponent(is,y) \
		(*(is) = (*(is) & ~0x0000FF00L) | (UWORD)((BYTE)(y) << 8))
#define SetInputDevType(is,t) \
		(*(is) = (*(is) & ~0x00030000L) | ((DWORD)((t) & DEVTYPE_MASK) << 16))
#define DEVICE_BUTTON1 ((INPUT_STATE)1 << 19)
#define DEVICE_BUTTON2 ((INPUT_STATE)1 << 20)
#define DEVICE_BUTTON3 ((INPUT_STATE)1 << 21)
#define DEVICE_PAUSE ((INPUT_STATE)1 << 22)
#define DEVICE_EXIT ((INPUT_STATE)1 << 23)
#define DEVICE_LEFTSHIFT ((INPUT_STATE)1 << 24)
#define DEVICE_RIGHTSHIFT ((INPUT_STATE)1 << 25)

extern BOOLEAN InitInput (UNICODE PauseKey, UNICODE ExitKey, BOOLEAN
		(*PauseFunc) (void));
extern BOOLEAN UninitInput (void);
extern INPUT_DEVICE CreateSerialKeyboardDevice (void);
extern INPUT_DEVICE CreateJoystickKeyboardDevice (UNICODE lfkey, UNICODE
		rtkey, UNICODE topkey, UNICODE botkey, UNICODE but1key, UNICODE
		but2key, UNICODE but3key, UNICODE shift1key, UNICODE shift2key);
extern INPUT_DEVICE CreateJoystickDevice (COUNT port);
extern INPUT_DEVICE CreateInternalDevice (INPUT_STATE (*input_func)
		(INPUT_REF InputRef, INPUT_STATE InputState));
extern BOOLEAN DestroyInputDevice (INPUT_DEVICE InputDevice);
extern INPUT_REF CaptureInputDevice (INPUT_DEVICE InputDevice);
extern INPUT_DEVICE ReleaseInputDevice (INPUT_REF InputRef);
extern INPUT_STATE GetInputState (INPUT_REF InputRef);
extern INPUT_STATE AnyButtonPress (BOOLEAN DetectSpecial);

extern void FlushInput (void);
extern int KeyDown (UNICODE which_scan);
extern UNICODE GetUnicodeKey (void);
extern UNICODE KBDToUNICODE (UNICODE which_key);
extern UNICODE KeyHit (void);
extern BYTE GetKeyState (UNICODE key);

extern BOOLEAN FindMouse (void);
extern void MoveMouse (SWORD x, SWORD y);
extern BYTE LocateMouse (PSWORD px, PSWORD py);

#endif /* _INPLIB_H */


