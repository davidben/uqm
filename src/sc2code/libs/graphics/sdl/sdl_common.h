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


/****************************************

  Star Control II: 3DO => SDL Port

  Copyright Toys for Bob, 2002

  Programmer: Chris Nelson
  
*****************************************/

#ifndef SDL_COMMON_H
#define SDL_COMMON_H

#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include "libs/graphics/gfxintrn.h"
#include "libs/input/inpintrn.h"
#include "libs/graphics/gfx_common.h"

#if defined (GFXMODULE_SDL_OPENGL)
#include "libs/graphics/sdl/opengl/opengl.h"
#elif defined (GFXMODULE_SDL_PURE)
#include "libs/graphics/sdl/pure/pure.h"
#endif

#include "libs/input/sdl/input.h"

#endif
