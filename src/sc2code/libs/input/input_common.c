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

#include "input_common.h"
#include "inpintrn.h"

INPUT_DEVICE
CreateSerialKeyboardDevice (void)
{
	INPUT_DEVICE InputDevice;

	if ((InputDevice = AllocInputDevice ()) != (INPUT_DEVICE)NULL_PTR)
	{
		INPUT_DESCPTR InputPtr;

		if ((InputPtr =
				LockInputDevice (InputDevice)) == (INPUT_DESCPTR)NULL_PTR)
		{
			FreeInputDevice (InputDevice);
			InputDevice = (INPUT_DEVICE)NULL_PTR;
		}
		else
		{
			SetInputDeviceHandle (InputPtr, InputDevice);
			SetInputDeviceInputFunc (InputPtr, _get_serial_keyboard_state);

			UnlockInputDevice (InputDevice);
		}
	}

	return (InputDevice);
}

INPUT_DEVICE
CreateJoystickKeyboardDevice (UNICODE lfkey, UNICODE rtkey, UNICODE
		topkey, UNICODE botkey, UNICODE but1key, UNICODE but2key, UNICODE
		but3key, UNICODE shift1key, UNICODE shift2key)
{
	INPUT_DEVICE InputDevice;

	if ((InputDevice = AllocInputDevice ()) != (INPUT_DEVICE)NULL_PTR)
	{
		INPUT_DESCPTR InputPtr;

		if ((InputPtr =
				LockInputDevice (InputDevice)) == (INPUT_DESCPTR)NULL_PTR)
		{
			FreeInputDevice (InputDevice);
			InputDevice = (INPUT_DEVICE)NULL_PTR;
		}
		else
		{
			SetInputDeviceHandle (InputPtr, InputDevice);
			SetInputDeviceInputFunc (InputPtr, _get_joystick_keyboard_state);
			SetInputDeviceKeyEquivalents (InputPtr,
					lfkey, rtkey, topkey, botkey,
					but1key, but2key, but3key,
					shift1key, shift2key);

			UnlockInputDevice (InputDevice);
		}
	}

	return (InputDevice);
}

INPUT_DEVICE
CreateJoystickDevice (COUNT port)
{
	INPUT_DEVICE InputDevice;

	if (!_joystick_port_active (port))
		InputDevice = (INPUT_DEVICE)NULL_PTR;
	else if ((InputDevice = AllocInputDevice ()) != (INPUT_DEVICE)NULL_PTR)
	{
		INPUT_DESCPTR InputPtr;

		if ((InputPtr =
				LockInputDevice (InputDevice)) == (INPUT_DESCPTR)NULL_PTR)
		{
			FreeInputDevice (InputDevice);
			InputDevice = (INPUT_DEVICE)NULL_PTR;
		}
		else
		{
			SetInputDeviceHandle (InputPtr, InputDevice);
			SetInputDeviceInputFunc (InputPtr, _get_joystick_state);
			SetInputDeviceJoystickPort (InputPtr, port);

			UnlockInputDevice (InputDevice);
		}
	}

	return (InputDevice);
}

INPUT_DEVICE
CreateInternalDevice (INPUT_STATE (*input_func) (INPUT_REF InputRef,
		INPUT_STATE InputState))
{
	INPUT_DEVICE InputDevice;

	if (input_func == NULL_PTR)
		InputDevice = (INPUT_DEVICE)NULL_PTR;
	else if ((InputDevice = AllocInputDevice ()) != (INPUT_DEVICE)NULL_PTR)
	{
		INPUT_DESCPTR InputPtr;

		if ((InputPtr =
				LockInputDevice (InputDevice)) == (INPUT_DESCPTR)NULL_PTR)
		{
			FreeInputDevice (InputDevice);
			InputDevice = (INPUT_DEVICE)NULL_PTR;
		}
		else
		{
			SetInputDeviceHandle (InputPtr, InputDevice);
			SetInputDeviceInputFunc (InputPtr, input_func);

			UnlockInputDevice (InputDevice);
		}
	}

	return (InputDevice);
}

INPUT_REF
CaptureInputDevice (INPUT_DEVICE InputDevice)
{
	if (InputDevice != (INPUT_DEVICE)NULL_PTR)
		return ((INPUT_REF)LockInputDevice (InputDevice));

	return ((INPUT_REF)NULL_PTR);
}

INPUT_DEVICE
ReleaseInputDevice (INPUT_REF InputRef)
{
	INPUT_DEVICE InputDevice;

	if (InputRef == (INPUT_REF)NULL_PTR)
		InputDevice = (INPUT_DEVICE)NULL_PTR;
	else
	{
		InputDevice = GetInputDeviceHandle (InputRef);
		UnlockInputDevice (InputDevice);
	}

	return (InputDevice);
}

BOOLEAN
DestroyInputDevice (INPUT_DEVICE InputDevice)
{
	if (InputDevice != (INPUT_DEVICE)NULL_PTR)
		return (FreeInputDevice (InputDevice));

	return (FALSE);
}

INPUT_STATE
GetInputState (INPUT_REF InputRef)
{
	if (InputRef == 0)
		return (_get_pause_exit_state ());
	else
		return ((*GetInputDeviceInputFunc (InputRef))
				(InputRef, _get_pause_exit_state ()));
}

