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

#include "strintrn.h"
#include "libs/graphics/gfx_common.h"
#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>

static void
dword_convert (PDWORD dword_array, COUNT num_dwords)
{
	PBYTE p = (PBYTE)dword_array;

	do
	{
		*dword_array++ = MAKE_DWORD (
				MAKE_WORD (p[3], p[2]),
				MAKE_WORD (p[1], p[0])
				);
		p += 4;
	} while (--num_dwords);
}

MEM_HANDLE
_GetStringData (FILE *fp, DWORD length)
{
	MEM_HANDLE hData;

	{
		char *s;
		extern char *_cur_resfile_name;

		if (_cur_resfile_name && (s = strrchr (_cur_resfile_name, '.')) && stricmp (s, ".txt") == 0)
		{
#define MAX_STRINGS 2048
#define POOL_SIZE 4096
#ifdef WIN32
			int omode;
#endif
			int n, path_len;
			DWORD opos,
						slen[MAX_STRINGS], StringOffs, tot_string_size,
						clen[MAX_STRINGS], ClipOffs, tot_clip_size;
			char CurrentLine[1024], clip_path[1024], *strdata, *clipdata;

			if ((strdata = HMalloc (tot_string_size = POOL_SIZE)) == 0)
				return (0);

			if ((clipdata = HMalloc (tot_clip_size = POOL_SIZE)) == 0)
			{
				HFree (strdata);
				return (0);
			}

			{
				char *s1, *s2;

				if (((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
								&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0)
					n = 0;
				else
				{
					if (s2 > s1)
						s1 = s2;
					n = s1 - _cur_resfile_name + 1;
					strncpy (clip_path, _cur_resfile_name, n);
				}
				clip_path[n] = '\0';
				path_len = strlen (clip_path);
			}

			opos = ftell (fp);
#ifdef WIN32
			omode = _setmode (fileno (fp), O_TEXT);
#endif
			n = -1;
			StringOffs = ClipOffs = 0;
			while (fgets (CurrentLine, sizeof (CurrentLine), fp) && n < MAX_STRINGS - 1)
			{
				int l;

				if (CurrentLine[0] == '#')
				{
					char CopyLine[1024];

					strcpy (CopyLine, CurrentLine);
					s = strtok (&CopyLine[1], "()");
					if (s)
					{
						if (n >= 0)
						{
							while (slen[n] > 1 && strdata[StringOffs - 2] == '\n')
							{
								--slen[n];
								--StringOffs;
								strdata[StringOffs - 1] = '\0';
							}
						}

						slen[++n] = 0;
						clen[n] = 0;
						s = strtok (NULL, " \t\n)");
						if (s)
						{
							l = path_len + strlen (s) + 1;
							if (ClipOffs + l > tot_clip_size
									&& (clipdata = HRealloc (clipdata,
											tot_clip_size += POOL_SIZE)) == 0)
							{
								HFree (strdata);
								return (0);
							}

							strcpy (&clipdata[ClipOffs], clip_path);
							strcpy (&clipdata[ClipOffs + path_len], s);
							ClipOffs += l;
							clen[n] = l;
						}
					}
				}
				else if (n >= 0)
				{
					l = strlen (CurrentLine) + 1;
					if (StringOffs + l > tot_string_size
							&& (strdata = HRealloc (strdata,
									tot_string_size += POOL_SIZE)) == 0)
					{
						HFree (clipdata);
						return (0);
					}

					if (slen[n])
					{
						--slen[n];
						--StringOffs;
					}
					s = &strdata[StringOffs];
					slen[n] += l;
					StringOffs += l;

					strcpy (s, CurrentLine);
				}

				if ((int)ftell (fp) - (int)opos >= (int)length)
					break;
			}
#ifdef WIN32
			_setmode (fileno (fp), omode);
#endif
			if (n >= 0)
			{
				while (slen[n] > 1 && strdata[StringOffs - 2] == '\n')
				{
					--slen[n];
					--StringOffs;
					strdata[StringOffs - 1] = '\0';
				}
			}

			hData = 0;
			if (++n && (hData = AllocStringTable (
							(sizeof (STRING_TABLE_DESC) - sizeof (DWORD))
									+ (sizeof (DWORD) * ((n + 1) << (ClipOffs ? 1 : 0)))
									+ StringOffs
									+ ClipOffs
							)))
			{
				PDWORD lpStringOffs, lpClipOffs;
				STRING_TABLEPTR lpST;

				LockStringTable (hData, &lpST);
				lpST->StringCount = n;
				lpST->flags = 0;
				if (ClipOffs)
					lpST->flags |= HAS_SOUND_CLIPS;

				memcpy (&lpST->StringOffsets[(n + 1) << (ClipOffs ? 1 : 0)], strdata, StringOffs);
				memcpy ((BYTE *)&lpST->StringOffsets[(n + 1) << (ClipOffs ? 1 : 0)] + StringOffs,
						clipdata, ClipOffs);
				ClipOffs = ((BYTE *)&lpST->StringOffsets[(n + 1) << (ClipOffs ? 1 : 0)]
								- (BYTE *)lpST) + StringOffs;
				StringOffs = ClipOffs - StringOffs;

				for (n = 0, lpStringOffs = lpST->StringOffsets,
								lpClipOffs = &lpST->StringOffsets[lpST->StringCount + 1];
						n < (int)lpST->StringCount; ++n, ++lpStringOffs, ++lpClipOffs)
				{
					*lpStringOffs = StringOffs;
					StringOffs += slen[n];
					if (lpST->flags & HAS_SOUND_CLIPS)
					{
						*lpClipOffs = ClipOffs;
						ClipOffs += clen[n];
					}
				}
				*lpStringOffs = StringOffs;
				if (lpST->flags & HAS_SOUND_CLIPS)
					*lpClipOffs = ClipOffs;

				UnlockStringTable (hData);
			}

			HFree (strdata);

			return (hData);
		}
	}

	hData = GetResourceData (fp, length, MEM_SOUND);
	if (hData)
	{
		COUNT StringCount;
		DWORD StringOffs;
		PDWORD lpStringOffs;
		STRING_TABLEPTR lpST;

		LockStringTable (hData, &lpST);
length = *(DWORD *)&lpST->StringCount;
dword_convert (&length, 1);
lpST->StringCount = (unsigned short)length;
		StringCount = lpST->StringCount;
		lpST->flags = 0;

		lpStringOffs = lpST->StringOffsets;
dword_convert (lpStringOffs, StringCount + 1);
		StringOffs = sizeof (STRING_TABLE_DESC)
				+ (sizeof (DWORD) * StringCount);
		do
		{
			StringOffs += *lpStringOffs;
			*lpStringOffs++ = StringOffs;
		} while (StringCount--);

		UnlockStringTable (hData);
	}

	return (hData);
}


