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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * This file stores some temporary definitions relating to strings.
 * Some history of these definitions:
 * At some point when Chris Nelson was working at Toys for Bob on the
 * code which would later be released as UQM, a lot of "char *" strings
 * were changed to "UNICODE", and string operations like strcpy() were
 * replaced by their wide-char variants. But this code wasn't ready yet,
 * so UNICODE was defined back to "char *", and the wide string operations
 * were defined to the regular ones.
 * I've created this file as I've got no better place to put these
 * definitions now. I expect that somewhere in the future I'll change
 * everything back to "char *" and use UTF-8 internally. - SvdB
 */

#ifndef _STRTEMP_H
#define _STRTEMP_H

#define wsprintf sprintf
#define wstrlen strlen
#define wstrcpy strcpy
#define wstrcat strcat
#define wstrupr strupr
#define wstrncpy strncpy
#define wstricmp stricmp
#define wstrtoul strtoul
#define wstrcspn strcspn

#endif  /* _STRTEMP_H */


