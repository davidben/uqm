//Copyright Michael Martin, 2006

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

#ifdef NETPLAY

#include "cnctdlg.h"
#include "controls.h"

typedef struct connect_dialog_state
{
	BOOLEAN (*InputFunc) (struct connect_dialog_state *pInputState);
	COUNT MenuRepeatDelay;

	BOOLEAN Initialized;
	int which_side;
	
	int option;
} CONNECT_DIALOG_STATE;

typedef CONNECT_DIALOG_STATE *PCONNECT_DIALOG_STATE;

static BOOLEAN
DoMeleeConnectDialog (PCONNECT_DIALOG_STATE state)
{
	if (!state->Initialized)
	{
		state->Initialized = TRUE;
		SetDefaultMenuRepeatDelay ();
		/* Prepare widgets, draw stuff, etc. */
	}

	/* But this is skeletal, so just end immediately */
	return FALSE;
}

BOOLEAN
MeleeConnectDialog (int side)
{
	CONNECT_DIALOG_STATE state;

	state.Initialized = FALSE;
	state.which_side = side;
	state.InputFunc = DoMeleeConnectDialog;

	DoInput ((PVOID)&state, TRUE);

	return TRUE;
}

#endif /* NETPLAY */
