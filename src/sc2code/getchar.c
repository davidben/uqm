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


static UWORD cur_char = ' ';

void
SetJoystickChar (UWORD which_char)
{
	if (!isalnum (cur_char = which_char))
		cur_char = ' ';
}

UWORD
GetJoystickChar (void)
{
	int dy;
	UWORD old_char;

	if (PulsedInputState.key[KEY_MENU_SELECT])
		return ('\n');
	else if (PulsedInputState.key[KEY_MENU_CANCEL])
		return (0x1B);
	else if (PulsedInputState.key[KEY_MENU_SPECIAL])
		return (0x7F);
		
	old_char = cur_char;
	dy = 0;
	if (PulsedInputState.key[KEY_MENU_UP]) dy = -1;
	else if (PulsedInputState.key[KEY_MENU_DOWN]) dy = 1;
	if (dy)
	{
		if (cur_char == ' ')
		{
			if (dy > 0)
				cur_char = 'a';
			else /* if (dy < 0) */
				cur_char = '9';
		}
		else if (isdigit (cur_char))
		{
			cur_char += dy;
			if (cur_char < '0')
				cur_char = 'z';
			else if (cur_char > '9')
				cur_char = ' ';
		}
		else if (!isalpha (cur_char += dy))
		{
			cur_char -= dy;
			if (cur_char == 'a' || cur_char == 'A')
				cur_char = ' ';
			else
				cur_char = '0';
		}
	}
	
	if ((PulsedInputState.key[KEY_MENU_PAGE_UP] || PulsedInputState.key[KEY_MENU_PAGE_DOWN]) && isalpha (cur_char))
	{
		if (islower (cur_char))
			cur_char = toupper (cur_char);
		else
			cur_char = tolower (cur_char);
	}
	
	return (cur_char == old_char ? 0 : cur_char);
}

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

		DoInput (pTES, TRUE);

		if (pTES->CacheStr)
			HFree (pTES->CacheStr);

		return pTES->Success;
	}

	pStr = pTES->InsPt;
	len = wstrlen (pStr);
	// save a copy of string
	CacheInsPt = pTES->InsPt;
	wstrncpy (pTES->CacheStr, pTES->BaseStr, pTES->MaxSize - 1);
	pTES->CacheStr[pTES->MaxSize - 1] = '\0';

	// process the pending character buffer
	while (ch = GetNextCharacter ())
	{
		if (isprint (ch) && (pStr + len - pTES->BaseStr + 1 < pTES->MaxSize))
		{
			memmove (pStr + 1, pStr, (len + 1) * sizeof (*pStr));
			*pStr = ch;
			++pStr;
			++len;
			changed = TRUE;
		}
	}

	if (!changed && PulsedInputState.key[KEY_CHARACTER])
	{	// keyboard repeat, but only when buffer empty
		ch = GetLastCharacter ();
		if (ch && isprint (ch) &&
				(pStr + len - pTES->BaseStr + 1 < pTES->MaxSize))
		{
			memmove (pStr + 1, pStr, (len + 1) * sizeof (*pStr));
			*pStr = ch;
			++pStr;
			++len;
			changed = TRUE;
		}
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
	else if (PulsedInputState.key[KEY_MENU_UP])
	{
		// do joystick text
	}
	else if (PulsedInputState.key[KEY_MENU_DOWN])
	{
		// do joystick text
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

