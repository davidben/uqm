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

#define NUM_INPUTS 16

typedef struct inp_dev
{
	INPUT_DEVICE ThisDevice;
	union
	{
		UNICODE key_equivalent[NUM_INPUTS];
		COUNT joystick_port;
	} device;
	INPUT_STATE (*input_func) (INPUT_REF InputRef, INPUT_STATE
			InputState);
} INPUT_DESC;
typedef INPUT_DESC *PINPUT_DESC;
typedef PINPUT_DESC INPUT_DESCPTR;

#define INPUT_DEVICE_PRIORITY DEFAULT_MEM_PRIORITY

typedef PBYTE BYTEPTR;

#define AllocInputDevice() \
		(INPUT_DEVICE)mem_allocate ((MEM_SIZE)sizeof (INPUT_DESC), \
		MEM_ZEROINIT, INPUT_DEVICE_PRIORITY, MEM_SIMPLE)
#define LockInputDevice(InputDevice) (INPUT_DESCPTR)mem_lock (InputDevice)
#define UnlockInputDevice(InputDevice) mem_unlock (InputDevice)
#define FreeInputDevice(InputDevice) mem_release (InputDevice)

#define SetInputDeviceHandle(i,h) \
		((INPUT_DESCPTR)(i))->ThisDevice = (h)
#define SetInputDeviceKeyEquivalents(i,l,r,t,b,b1,b2,b3,s1,s2) \
		(((INPUT_DESCPTR)(i))->device.key_equivalent[0] = KBDToUNICODE (l), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[1] = KBDToUNICODE (r), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[2] = KBDToUNICODE (t), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[3] = KBDToUNICODE (b), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[4] = KBDToUNICODE (b1), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[5] = KBDToUNICODE (b2), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[6] = KBDToUNICODE (b3), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[7] = KBDToUNICODE (s1), \
		((INPUT_DESCPTR)(i))->device.key_equivalent[8] = KBDToUNICODE (s2))
#define SetInputDeviceJoystickPort(i,p) \
		((INPUT_DESCPTR)(i))->device.joystick_port = (p)
#define SetInputDeviceInputFunc(i,f) \
		((INPUT_DESCPTR)(i))->input_func = (f)

#define GetInputDeviceHandle(i) \
		((INPUT_DESCPTR)(i))->ThisDevice
#define GetInputDeviceKeyEquivalentPtr(i) \
		((INPUT_DESCPTR)(i))->device.key_equivalent
#define GetInputDeviceJoystickPort(i) \
		((INPUT_DESCPTR)(i))->device.joystick_port
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
