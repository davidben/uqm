#ifndef _VCONTROL_H_
#define _VCONTROL_H_

#include "port.h"
#include "libs/uio.h"
#include SDL_INCLUDE(SDL.h)

/* Initialization routines */
void VControl_Init (void);
void VControl_Uninit (void);

/* Control of bindings */
int  VControl_AddKeyBinding (SDLKey symbol, int *target);
void VControl_RemoveKeyBinding (SDLKey symbol, int *target);
int  VControl_AddJoyAxisBinding (int port, int axis, int polarity, int *target);
void VControl_RemoveJoyAxisBinding (int port, int axis, int polarity, int *target);
int  VControl_SetJoyThreshold (int port, int threshold);
int  VControl_AddJoyButtonBinding (int port, int button, int *target);
void VControl_RemoveJoyButtonBinding (int port, int button, int *target);
int  VControl_AddJoyHatBinding (int port, int which, Uint8 dir, int *target);
void VControl_RemoveJoyHatBinding (int port, int which, Uint8 dir, int *target);

void VControl_RemoveAllBindings (void);

/* The listener.  Routines besides HandleEvent may be used to 'fake' inputs without 
 * fabricating an SDL_Event. 
 */
void VControl_HandleEvent (const SDL_Event *e);
void VControl_ProcessKeyDown (SDLKey symbol);
void VControl_ProcessKeyUp (SDLKey symbol);
void VControl_ProcessJoyButtonDown (int port, int button);
void VControl_ProcessJoyButtonUp (int port, int button);
void VControl_ProcessJoyAxis (int port, int axis, int value);
void VControl_ProcessJoyHat (int port, int which, Uint8 value);

/* Force the input into the blank state.  For preventing "sticky" keys. */
void VControl_ResetInput (void);

/* Name control.  To provide a table of names and bindings, declare
 * a persistent, unchanging array of VControl_NameBinding and end it
 * with a {0, 0} entry.  Pass this array to VControl_RegisterNameTable.
 * Only one name table may be registered at a time; subsequent calls
 * replace the previous values. */

typedef struct vcontrol_namebinding {
	char *name;
	int *target;
} VControl_NameBinding;

void VControl_RegisterNameTable (VControl_NameBinding *table);

/* Version number control */
int VControl_GetConfigFileVersion (void);
void VControl_SetConfigFileVersion (int v);

/* Dump a configuration file corresponding to the current bindings and names. */
void VControl_Dump (FILE *out);
/* Read a configuration file.  Returns number of errors encountered. */
int VControl_ReadConfiguration (uio_Stream *in);
int VControl_GetErrorCount (void);
int VControl_GetValidCount (void);

/* Iterator control.  Start an iteration with StartIter or
   StartIterByName; then call NextBindingName with a char array of at
   least 40 bytes in size until it returns 0.  Produces names. */

void VControl_StartIter (int *target);
void VControl_StartIterByName (char *targetname);
int  VControl_NextBindingName (char *buffer);

/* Tracking the "last interesting event."  Used to poll to find new
   control keys. */

void VControl_ClearEvent (void);
int  VControl_GetLastEvent (SDL_Event *e);

#endif
