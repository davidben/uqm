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

#include <ctype.h>
#include "controls.h"
#include "libs/inplib.h"
#include "libs/misc.h"
#include "globdata.h"
#include "sounds.h"
#include "settings.h"
#include "resinst.h"
#include "nameref.h"


BOOLEAN
DoTextEntry (PTEXTENTRY_STATE pTES)
{
	UNICODE ch;
	UNICODE *pStr;
	UNICODE *CacheInsPt;
	int len;
	BOOLEAN changed = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (!pTES->Initialized)
	{	// init basic vars
		pTES->InputFunc = DoTextEntry;
		pTES->InsPt = pTES->BaseStr;
		pTES->CacheStr = HMalloc (pTES->MaxSize);
		pTES->Success = FALSE;
		pTES->Initialized = TRUE;
		pTES->JoystickMode = FALSE;
		pTES->UpperRegister = TRUE;
		pTES->JoyAlphaString = CaptureStringTable (
				LoadStringTable (JOYSTICK_ALPHA_STRTAB));
		pTES->JoyAlphaStr = GetStringAddress (
				SetAbsStringTableIndex (pTES->JoyAlphaString, 0));

		DoInput (pTES, TRUE);

		if (pTES->CacheStr)
			HFree (pTES->CacheStr);
		DestroyStringTable ( ReleaseStringTable (pTES->JoyAlphaString));

		return pTES->Success;
	}

	pStr = pTES->InsPt;
	len = wstrlen (pStr);
	// save a copy of string
	CacheInsPt = pTES->InsPt;
	wstrncpy (pTES->CacheStr, pTES->BaseStr, pTES->MaxSize - 1);
	pTES->CacheStr[pTES->MaxSize - 1] = '\0';

	// process the pending character buffer
	ch = GetNextCharacter ();
	if (!ch && PulsedInputState.key[KEY_CHARACTER])
	{	// keyboard repeat, but only when buffer empty
		ch = GetLastCharacter ();
	}
	while (ch)
	{
		pTES->JoystickMode = FALSE;

		if (isprint (ch) && (pStr + len - pTES->BaseStr + 1 < pTES->MaxSize))
		{
			memmove (pStr + 1, pStr, (len + 1) * sizeof (*pStr));
			*pStr = ch;
			++pStr;
			++len;
			changed = TRUE;
		}
		ch = GetNextCharacter ();
	}

	if (PulsedInputState.key[KEY_MENU_DELETE])
	{
		if (len)
		{
			memmove (pStr, pStr + 1, len * sizeof (*pStr));
			--len;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.key[KEY_MENU_BACKSPACE])
	{
		if (pStr > pTES->BaseStr)
		{
			memmove (pStr - 1, pStr, (len + 1) * sizeof (*pStr));
			--pStr;
			--len;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.key[KEY_MENU_LEFT])
	{
		if (pStr > pTES->BaseStr)
		{
			--pStr;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.key[KEY_MENU_RIGHT])
	{
		if (len > 0)
		{
			++pStr;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.key[KEY_MENU_HOME])
	{
		if (pStr > pTES->BaseStr)
		{
			pStr = pTES->BaseStr;
			changed = TRUE;
		}
	}
	else if (PulsedInputState.key[KEY_MENU_END])
	{
		if (len > 0)
		{
			pStr += len;
			changed = TRUE;
		}
	}
	
	if (pTES->JoyAlphaStr && (
			PulsedInputState.key[KEY_MENU_UP] ||
			PulsedInputState.key[KEY_MENU_DOWN] ||
			PulsedInputState.key[KEY_MENU_PAGE_UP] ||
			PulsedInputState.key[KEY_MENU_PAGE_DOWN]) )
	{	// do joystick text
		UNICODE newch;
		UNICODE cmpch;
		int i;

		pTES->JoystickMode = TRUE;

		if (len)
			ch = *pStr;
		else
			ch = pTES->JoyAlphaStr[0];
		
		newch = ch;
		cmpch = toupper (ch);

		// find current char in the alphabet
		for (i = 0; pTES->JoyAlphaStr[i] != cmpch
				&& pTES->JoyAlphaStr[i] != '\0'; ++i)
			;

		if (PulsedInputState.key[KEY_MENU_UP])
		{
			--i;
			if (i < 0)
				i = wstrlen (pTES->JoyAlphaStr) - 1;
			newch = pTES->JoyAlphaStr[i];
		}
		else if (PulsedInputState.key[KEY_MENU_DOWN])
		{
			++i;
			if (i >= (int)wstrlen(pTES->JoyAlphaStr))
				newch = pTES->JoyAlphaStr[0];
			else
				newch = pTES->JoyAlphaStr[i];
		}

		if (PulsedInputState.key[KEY_MENU_PAGE_UP] ||
				PulsedInputState.key[KEY_MENU_PAGE_DOWN])
		{
			if (len)
			{	// single char change
				if (islower (newch))
					newch = toupper (newch);
				else
					newch = tolower (newch);
			}
			else
			{	// register change
				pTES->UpperRegister = !pTES->UpperRegister;
			}
		}
		else
		{	// check register
			if (pTES->UpperRegister)
				newch = toupper (newch);
			else
				newch = tolower (newch);
		}

		if (newch != ch)
		{
			if (len)
			{	// change current
				pStr[0] = newch;
				changed = TRUE;
			}
			else if (pStr + len - pTES->BaseStr + 1 < pTES->MaxSize)
			{	// append
				pStr[0] = newch;
				pStr[1] = '\0';
				++len;
				changed = TRUE;
			}
		}
	}
	
	if (PulsedInputState.key[KEY_MENU_SELECT])
	{	// done entering
		pTES->Success = TRUE;
		return FALSE;
	}
	else if (PulsedInputState.key[KEY_MENU_EDIT_CANCEL])
	{	// canceled entering
		pTES->Success = FALSE;
		return FALSE;
	}

	pTES->InsPt = pStr;

	if (changed && pTES->ChangeCallback)
	{
		if (!pTES->ChangeCallback (pTES))
		{	// changes not accepted - revert
			wstrncpy (pTES->BaseStr, pTES->CacheStr, pTES->MaxSize - 1);
			pTES->BaseStr[pTES->MaxSize - 1] = '\0';
			pTES->InsPt = CacheInsPt;

			PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 2),
					0, NotPositional (), NULL, 0);
		}
	}
		
	return TRUE;
}

