#ifndef _SDLUIO_H
#define _SDLUIO_H

#include "port.h"
#include "libs/uio.h"
#include SDL_INCLUDE(SDL.h)
#include SDL_INCLUDE(SDL_rwops.h)

SDL_Surface *sdluio_loadImage (uio_DirHandle *dir, const char *fileName);
int sdluio_seek (SDL_RWops *context, int offset, int whence);
int sdluio_read (SDL_RWops *context, void *ptr, int size, int maxnum);
int sdluio_write (SDL_RWops *context, const void *ptr, int size, int num);
int sdluio_close (SDL_RWops *context);


#endif  /* _SDLUIO_H */

