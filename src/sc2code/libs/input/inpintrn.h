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

#ifndef _INPINTRN_H
#define _INPINTRN_H

#include "inplib.h"

#define NUM_INPUTS 10
/*
 * Inputs
 * 0 - Left
 * 1 - Right
 * 2 - Up
 * 3 - Down
 * 4 - Button 1
 * 5 - Button 2
 * 6 - Button 3
 * 7 - Button 4
 * 8 - Left shift
 * 9 - Right shift
 */

typedef struct inp_dev
{
	INPUT_DEVICE ThisDevice;
	union
	{
		UWORD key_equivalent[NUM_INPUTS];
		struct
		{
			BYTE joystick_port;
			UWORD joystick_threshold;
			/* See inplib.h for description of this field */
			UWORD joystick_buttons[NUM_INPUTS];
		} joystick_info;
	} device;
	INPUT_STATE (*input_func) (INPUT_REF InputRef, INPUT_STATE
			InputState);
} INPUT_DESC;
typedef INPUT_DESC *PINPUT_DESC;
typedef PINPUT_DESC INPUT_DESCPTR;

#define INPUT_DEVICE_PRIORITY DEFAULT_MEM_PRIORITY

#ifndef _DISPLIST_H
/* This ifndef is temporary to avoid a collision with displist.h
   This shouldn't be necessary, but SDL_wrappper.h isn't set up correctly
   at the moment. */
typedef PBYTE BYTEPTR;
#endif

#define AllocInputDevice() \
		(INPUT_DEVICE)mem_allocate ((MEM_SIZE)sizeof (INPUT_DESC), \
		MEM_ZEROINIT, INPUT_DEVICE_PRIORITY, MEM_SIMPLE)
#define LockInputDevice(InputDevice) (INPUT_DESCPTR)mem_lock (InputDevice)
#define UnlockInputDevice(InputDevice) mem_unlock (InputDevice)
#define FreeInputDevice(InputDevice) mem_release (InputDevice)

#define SetInputDeviceHandle(i,h) \
		((INPUT_DESCPTR)(i))->ThisDevice = (h)
#define SetInputDeviceKeyEquivalents(i,l,r,t,b,b1,b2,b3,b4,s1,s2) \
		(((INPUT_DESCPTR)(i))->device.key_equivalent[0] = (l), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[1] = (r), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[2] = (t), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[3] = (b), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[4] = (b1), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[5] = (b2), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[6] = (b3), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[7] = (b4), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[8] = (s1), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[9] = (s2))
#define SetInputDeviceJoystickButtons(i,l,r,t,b,b1,b2,b3,b4,s1,s2) \
		(((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[0] = l, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[1] = r, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[2] = t, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[3] = b, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[4] = b1, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[5] = b2, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[6] = b3, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[7] = b4, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[8] = s1, \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons[9] = s2)
#define SetInputDeviceJoystickPort(i,p) \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_port = (p)
#define SetInputDeviceJoystickThreshold(i,t) \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_threshold = (t)
#define SetInputDeviceInputFunc(i,f) \
		((INPUT_DESCPTR)(i))->input_func = (f)

#define GetInputDeviceHandle(i) \
		((INPUT_DESCPTR)(i))->ThisDevice
#define GetInputDeviceKeyEquivalentPtr(i) \
		((INPUT_DESCPTR)(i))->device.key_equivalent
#define GetInputDeviceJoystickButtonPtr(i) \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_buttons
#define GetInputDeviceJoystickPort(i) \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_port
#define GetInputDeviceJoystickThreshold(i) \
		((INPUT_DESCPTR)(i))->device.joystick_info.joystick_threshold
#define GetInputDeviceInputFunc(i) \
		((INPUT_DESCPTR)(i))->input_func

extern INPUT_STATE _get_serial_keyboard_state (INPUT_REF InputRef,
		INPUT_STATE InputState);
extern INPUT_STATE _get_joystick_keyboard_state (INPUT_REF InputRef,
		INPUT_STATE InputState);
extern INPUT_STATE _get_joystick_state (INPUT_REF InputRef,
		INPUT_STATE InputState);
extern BOOLEAN _joystick_port_active (COUNT port);
extern INPUT_STATE _get_pause_exit_state (void);

#endif /* _INPINTRN_H */
