#include <SDL.h>

#ifndef _VCONTROL_H_
#define _VCONTROL_H_


/* Initialization routines */
void VControl_Init (void);
void VControl_Uninit (void);

/* Control of bindings */
void VControl_AddKeyBinding (int symbol, int *target);
void VControl_RemoveKeyBinding (int symbol, int *target);
void VControl_AddJoyAxisBinding (int port, int axis, int polarity, int *target);
void VControl_RemoveJoyAxisBinding (int port, int axis, int polarity, int *target);
void VControl_SetJoyThreshold (int port, int threshold);
void VControl_AddJoyButtonBinding (int port, int button, int *target);
void VControl_RemoveJoyButtonBinding (int port, int button, int *target);
void VControl_AddJoyHatBinding (int port, int which, Uint8 dir, int *target);
void VControl_RemoveJoyHatBinding (int port, int which, Uint8 dir, int *target);

void VControl_RemoveAllBindings (void);

/* The listener.  Routines besides HandleEvent may be used to 'fake' inputs without 
 * fabricating an SDL_Event. 
 */
void VControl_HandleEvent (const SDL_Event *e);
void VControl_ProcessKeyDown (int symbol);
void VControl_ProcessKeyUp (int symbol);
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

typedef struct _vcontrol_namebinding {
	char *name;
	int *target;
} VControl_NameBinding;

void VControl_RegisterNameTable (VControl_NameBinding *table);

/* Dump a configuration file corresponding to the current bindings and names. */
void VControl_Dump (FILE *out);
/* Read a configuration file.  Returns number of errors encountered. */
int VControl_ReadConfiguration (FILE *in);

#endif
