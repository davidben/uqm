#include "port.h"
#include SDL_INCLUDE(SDL.h)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vcontrol.h"
#include "vcontrol_malloc.h"
#include "keynames.h"

/* How many binding slots are allocated at once. */
#define POOL_CHUNK_SIZE 64

typedef struct vcontrol_keybinding {
	int *target;
	struct vcontrol_keypool *parent;
	struct vcontrol_keybinding *next;
} keybinding;

typedef struct vcontrol_keypool {
	keybinding pool[POOL_CHUNK_SIZE];
	int remaining;
	struct vcontrol_keypool *next;
} keypool;

typedef struct vcontrol_joystick_axis {
	keybinding *neg, *pos;
	int polarity;
} axis_type;

typedef struct vcontrol_joystick_hat {
	keybinding *left, *right, *up, *down;
	Uint8 last;
} hat_type;

typedef struct vcontrol_joystick {
	SDL_Joystick *stick;
	int numaxes, numbuttons, numhats;
	int threshold;
	axis_type *axes;
	keybinding **buttons;
	hat_type *hats;
} joystick;

static keybinding *bindings[SDLK_LAST];
static joystick *joysticks;
static int joycount;

static keypool *pool;
static VControl_NameBinding *nametable;

/* Statistics variables - set by VControl_ReadConfiguration */

static int version, errors, validlines;

/* Iterator temp variables */

static int *iter_target;
static int iter_device, iter_index;

/* Last interesting event */
static int event_ready;
static SDL_Event last_interesting;

static keypool *
allocate_key_chunk (void)
{
	keypool *x = vctrl_malloc (sizeof (keypool));
	if (x)
	{
		int i;
		x->remaining = POOL_CHUNK_SIZE;
		x->next = NULL;
		for (i = 0; i < POOL_CHUNK_SIZE; i++)
		{
			x->pool[i].target = NULL;
			x->pool[i].next = NULL;
			x->pool[i].parent = x;
		}
	}
	return x;
}

static void
free_key_pool (keypool *x)
{
	if (x)
	{
		free_key_pool (x->next);
		vctrl_free (x);
	}
}

static void
create_joystick (int index)
{
	SDL_Joystick *stick;
	int axes, buttons, hats;
	if (index >= joycount)
	{
		fprintf (stderr, "VControl warning: Tried to open a non-existent joystick!");
		return;
	}
	if (joysticks[index].stick)
	{
		// Joystick is already created.  Return.
		return;
	}
	stick = SDL_JoystickOpen (index);
	if (stick)
	{
		joystick *x = &joysticks[index];
		int j;
		fprintf (stderr, "VControl opened joystick: %s\n", SDL_JoystickName (index));
		axes = SDL_JoystickNumAxes (stick);
		buttons = SDL_JoystickNumButtons (stick);
		hats = SDL_JoystickNumHats (stick);
		fprintf (stderr, "%d axes, %d buttons, %d hats.\n", axes, buttons, hats);
		x->numaxes = axes;
		x->numbuttons = buttons;
		x->numhats = hats;
		x->axes = vctrl_malloc (sizeof (axis_type) * axes);
		x->buttons = vctrl_malloc (sizeof (keybinding *) * buttons);
		x->hats = vctrl_malloc (sizeof (hat_type) * hats);
		for (j = 0; j < axes; j++)
		{
			x->axes[j].neg = x->axes[j].pos = NULL;
		}
		for (j = 0; j < hats; j++)
		{
			x->hats[j].left = x->hats[j].right = NULL;
			x->hats[j].up = x->hats[j].down = NULL;
			x->hats[j].last = SDL_HAT_CENTERED;
		}
		for (j = 0; j < buttons; j++)
		{
			x->buttons[j] = NULL;
		}
		x->stick = stick;
	}
	else
	{
		fprintf (stderr, "VControl: Could not initialize joystick #%d\n", index);
	}
}
			
static void
destroy_joystick (int index)
{
	SDL_Joystick *stick = joysticks[index].stick;
	if (stick)
	{
		SDL_JoystickClose (stick);
		joysticks[index].stick = NULL;
		vctrl_free (joysticks[index].axes);
		vctrl_free (joysticks[index].buttons);
		vctrl_free (joysticks[index].hats);
		joysticks[index].numaxes = joysticks[index].numbuttons = 0;
		joysticks[index].axes = NULL;
		joysticks[index].buttons = NULL;
		joysticks[index].hats = NULL;
	}
}

static void
key_init (void)
{
	int i;
	pool = allocate_key_chunk ();
	for (i = 0; i < SDLK_LAST; i++)
		bindings[i] = NULL;
	/* Prepare for possible joystick controls.  We don't actually
	   GRAB joysticks unless we're asked to make a joystick
	   binding, though. */
	joycount = SDL_NumJoysticks ();
	if (joycount)
	{
		joysticks = vctrl_malloc (sizeof (joystick) * joycount);
		for (i = 0; i < joycount; i++)
		{
			joysticks[i].stick = NULL;	
			joysticks[i].numaxes = joysticks[i].numbuttons = 0;
			joysticks[i].axes = NULL;
			joysticks[i].buttons = NULL;
		}
	}
	else
	{
		joysticks = NULL;
	}
}

static void
key_uninit (void)
{
	int i;
	free_key_pool (pool);
	for (i = 0; i < SDLK_LAST; i++)
		bindings[i] = NULL;
	pool = NULL;
	for (i = 0; i < joycount; i++)
		destroy_joystick (i);
	vctrl_free (joysticks);
}

static void
name_init (void)
{
	nametable = NULL;
}

static void
name_uninit (void)
{
	nametable = NULL;
}

void
VControl_Init (void)
{
	key_init ();
	name_init ();
}

void
VControl_Uninit (void)
{
	key_uninit ();
	name_uninit ();
}

int
VControl_SetJoyThreshold (int port, int threshold)
{
	if (port >= 0 && port < joycount)
	{
		joysticks[port].threshold = threshold;
		return 0;
	}
	else
	{
		// fprintf (stderr, "VControl_SetJoyThreshold passed illegal port %d\n", port);
		return -1;
	}
}


static void
add_binding (keybinding **newptr, int *target)
{
	keybinding *newbinding;
	keypool *searchbase;
	int i;

	/* Acquire a pointer to the keybinding * that we'll be
	 * overwriting.  Along the way, ensure we haven't already
	 * bound this symbol to this target.  If we have, return.*/
	while (*newptr != NULL)
	{
		if ((*newptr)->target == target)
		{
			return;
		}
		newptr = &((*newptr)->next);
	}

	/* Now hunt through the binding pool for a free binding. */

	/* First, find a chunk with free spots in it */

	searchbase = pool;
	while (searchbase->remaining == 0)
	{
		/* If we're completely full, allocate a new chunk */
		if (searchbase->next == NULL)
		{
			searchbase->next = allocate_key_chunk ();
		}
		searchbase = searchbase->next;
	}

	/* Now find a free binding within it */

	newbinding = NULL;
	for (i = 0; i < POOL_CHUNK_SIZE; i++)
	{
		if (searchbase->pool[i].target == NULL)
		{
			newbinding = &searchbase->pool[i];
			break;
		}
	}

	/* Sanity check. */
	if (!newbinding)
	{
		fprintf (stderr, "add_binding failed to find a free binding slot!\n");
		return;
	}

	newbinding->target = target;
	newbinding->next = NULL;
	*newptr = newbinding;
	searchbase->remaining--;
}

static void
remove_binding (keybinding **ptr, int *target)
{
	if (!(*ptr))
	{
		/* Nothing bound to symbol; return. */
		return;
	}
	else if ((*ptr)->target == target)
	{
		keybinding *todel = *ptr;
		*ptr = todel->next;
		todel->target = NULL;
		todel->next = NULL;
		todel->parent->remaining++;
	}
	else
	{
		keybinding *prev = *ptr;
		while (prev->next != NULL)
		{
			if (prev->next->target == target)
			{
				keybinding *todel = prev->next;
				prev->next = todel->next;
				todel->target = NULL;
				todel->next = NULL;
				todel->parent->remaining++;
			}
		}
	}
}

static void
activate (keybinding *i)
{
	while (i != NULL)
	{
		*(i->target) = *(i->target)+1;
		i = i->next;
	}
}

static void
deactivate (keybinding *i)
{
	while (i != NULL)
	{
		if (*(i->target) > 0)
		{
			*(i->target) = *(i->target)-1;
		}
		i = i->next;
	}
}

int
VControl_AddBinding (SDL_Event *e, int *target)
{
	int result;
	switch (e->type)
	{
	case SDL_KEYDOWN:
		result = VControl_AddKeyBinding (e->key.keysym.sym, target);
		break;
	case SDL_JOYAXISMOTION:
		result = VControl_AddJoyAxisBinding (e->jaxis.which, e->jaxis.axis, (e->jaxis.value < 0) ? -1 : 1, target);
		break;
	case SDL_JOYHATMOTION:
		result = VControl_AddJoyHatBinding (e->jhat.which, e->jhat.hat, e->jhat.value, target);
		break;
	case SDL_JOYBUTTONDOWN:
		result = VControl_AddJoyButtonBinding (e->jbutton.which, e->jbutton.button, target);
		break;
	default:
		fprintf (stderr, "VControl_AddBinding didn't understand argument event\n");
		result = -1;
		break;
	}
	return result;
}

void
VControl_RemoveBinding (SDL_Event *e, int *target)
{
	switch (e->type)
	{
	case SDL_KEYDOWN:
		VControl_RemoveKeyBinding (e->key.keysym.sym, target);
		break;
	case SDL_JOYAXISMOTION:
		VControl_RemoveJoyAxisBinding (e->jaxis.which, e->jaxis.axis, (e->jaxis.value < 0) ? -1 : 1, target);
		break;
	case SDL_JOYHATMOTION:
		VControl_RemoveJoyHatBinding (e->jhat.which, e->jhat.hat, e->jhat.value, target);
		break;
	case SDL_JOYBUTTONDOWN:
		VControl_RemoveJoyButtonBinding (e->jbutton.which, e->jbutton.button, target);
		break;
	default:
		fprintf (stderr, "VControl_RemoveBinding didn't understand argument event\n");
		break;
	}
}

int
VControl_AddKeyBinding (SDLKey symbol, int *target)
{
	if ((symbol < 0) || (symbol >= SDLK_LAST)) {
		fprintf (stderr, "VControl: Illegal key index %d\n", symbol);
		return -1;
	}
	add_binding(&bindings[symbol], target);
	return 0;
}

void
VControl_RemoveKeyBinding (SDLKey symbol, int *target)
{
	if ((symbol < 0) || (symbol >= SDLK_LAST)) {
		fprintf (stderr, "VControl: Illegal key index %d\n", symbol);
		return;
	}
	remove_binding (&bindings[symbol], target);
}

int
VControl_AddJoyAxisBinding (int port, int axis, int polarity, int *target)
{
	if (port >= 0 && port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((axis >= 0) && (axis < j->numaxes))
		{
			if (polarity < 0)
			{
				add_binding(&joysticks[port].axes[axis].neg, target);
			}
			else if (polarity > 0)
			{
				add_binding(&joysticks[port].axes[axis].pos, target);
			}
			else
			{
				// fprintf (stderr, "VControl: Attempted to bind to polarity zero\n");
				return -1;
			}
		}
		else
		{
			// fprintf (stderr, "VControl: Attempted to bind to illegal axis %d\n", axis);
			return -1;
		}
	}
	else
	{
		// fprintf (stderr, "VControl: Attempted to bind to illegal port %d\n", port);
		return -1;
	}
	return 0;
}

void
VControl_RemoveJoyAxisBinding (int port, int axis, int polarity, int *target)
{
	if (port >= 0 && port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((axis >= 0) && (axis < j->numaxes))
		{
			if (polarity < 0)
			{
				remove_binding(&joysticks[port].axes[axis].neg, target);
			}
			else if (polarity > 0)
			{
				remove_binding(&joysticks[port].axes[axis].pos, target);
			}
			else
			{
				fprintf (stderr, "VControl: Attempted to unbind from polarity zero\n");
			}
		}
		else
		{
			fprintf (stderr, "VControl: Attempted to unbind from illegal axis %d\n", axis);
		}
	}
	else
	{
		fprintf (stderr, "VControl: Attempted to unbind from illegal port %d\n", port);
	}
}

int
VControl_AddJoyButtonBinding (int port, int button, int *target)
{
	if (port >= 0 && port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((button >= 0) && (button < j->numbuttons))
		{
			add_binding(&joysticks[port].buttons[button], target);
			return 0;
		}
		else
		{
			// fprintf (stderr, "VControl: Attempted to bind to illegal button %d\n", button);
			return -1;
		}
	}
	else
	{
		// fprintf (stderr, "VControl: Attempted to bind to illegal port %d\n", port);
		return -1;
	}
}

void
VControl_RemoveJoyButtonBinding (int port, int button, int *target)
{
	if (port >= 0 && port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((button >= 0) && (button < j->numbuttons))
		{
			remove_binding (&joysticks[port].buttons[button], target);
		}
		else
		{
			fprintf (stderr, "VControl: Attempted to unbind from illegal button %d\n", button);
		}
	}
	else
	{
		fprintf (stderr, "VControl: Attempted to unbind from illegal port %d\n", port);
	}
}

int
VControl_AddJoyHatBinding (int port, int which, Uint8 dir, int *target)
{
	if (port >= 0 && port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((which >= 0) && (which < j->numhats))
		{
			if (dir == SDL_HAT_LEFT)
			{
				add_binding(&joysticks[port].hats[which].left, target);
			}
			else if (dir == SDL_HAT_RIGHT)
			{
				add_binding(&joysticks[port].hats[which].right, target);
			}
			else if (dir == SDL_HAT_UP)
			{
				add_binding(&joysticks[port].hats[which].up, target);
			}
			else if (dir == SDL_HAT_DOWN)
			{
				add_binding(&joysticks[port].hats[which].down, target);
			}
			else
			{
				// fprintf (stderr, "VControl: Attempted to bind to illegal direction\n");
				return -1;
			}
			return 0;
		}
		else
		{
			// fprintf (stderr, "VControl: Attempted to bind to illegal hat %d\n", which);
			return -1;
		}
	}
	else
	{
		// fprintf (stderr, "VControl: Attempted to bind to illegal port %d\n", port);
		return -1;
	}
}

void
VControl_RemoveJoyHatBinding (int port, int which, Uint8 dir, int *target)
{
	if (port >= 0 && port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((which >= 0) && (which < j->numhats))
		{
			if (dir == SDL_HAT_LEFT)
			{
				remove_binding(&joysticks[port].hats[which].left, target);
			}
			else if (dir == SDL_HAT_RIGHT)
			{
				remove_binding(&joysticks[port].hats[which].right, target);
			}
			else if (dir == SDL_HAT_UP)
			{
				remove_binding(&joysticks[port].hats[which].up, target);
			}
			else if (dir == SDL_HAT_DOWN)
			{
				remove_binding(&joysticks[port].hats[which].down, target);
			}
			else
			{
				fprintf (stderr, "VControl: Attempted to unbind from illegal direction\n");
			}
		}
		else
		{
			fprintf (stderr, "VControl: Attempted to unbind from illegal hat %d\n", which);
		}
	}
	else
	{
		fprintf (stderr, "VControl: Attempted to unbind from illegal port %d\n", port);
	}
}

void
VControl_RemoveAllBindings ()
{
	key_uninit ();
	key_init ();
}

void
VControl_ProcessKeyDown (SDLKey symbol)
{
	activate (bindings[symbol]);
}

void
VControl_ProcessKeyUp (SDLKey symbol)
{
	deactivate (bindings[symbol]);
}

void
VControl_ProcessJoyButtonDown (int port, int button)
{
	if (!joysticks[port].stick)
		return;
	activate (joysticks[port].buttons[button]);
}

void
VControl_ProcessJoyButtonUp (int port, int button)
{
	if (!joysticks[port].stick)
		return;
	deactivate (joysticks[port].buttons[button]);
}

void
VControl_ProcessJoyAxis (int port, int axis, int value)
{
	int t;
	if (!joysticks[port].stick)
		return;
	t = joysticks[port].threshold;
	if (value > t)
	{
		if (joysticks[port].axes[axis].polarity != 1)
		{
			if (joysticks[port].axes[axis].polarity == -1)
			{
				deactivate (joysticks[port].axes[axis].neg);
			}
			joysticks[port].axes[axis].polarity = 1;
			activate (joysticks[port].axes[axis].pos);
		}
	}
	else if (value < -t)
	{
		if (joysticks[port].axes[axis].polarity != -1)
		{
			if (joysticks[port].axes[axis].polarity == 1)
			{
				deactivate (joysticks[port].axes[axis].pos);
			}
			joysticks[port].axes[axis].polarity = -1;
			activate (joysticks[port].axes[axis].neg);
		}
	}
	else
	{
		if (joysticks[port].axes[axis].polarity == -1)
		{
			deactivate (joysticks[port].axes[axis].neg);
		}
		else if (joysticks[port].axes[axis].polarity == 1)
		{
			deactivate (joysticks[port].axes[axis].pos);
		}
		joysticks[port].axes[axis].polarity = 0;
	}
}

void
VControl_ProcessJoyHat (int port, int which, Uint8 value)
{
	Uint8 old;
	if (!joysticks[port].stick)
		return;
	old = joysticks[port].hats[which].last;
	if (!(old & SDL_HAT_LEFT) && (value & SDL_HAT_LEFT))
		activate (joysticks[port].hats[which].left);
	if (!(old & SDL_HAT_RIGHT) && (value & SDL_HAT_RIGHT))
		activate (joysticks[port].hats[which].right);
	if (!(old & SDL_HAT_UP) && (value & SDL_HAT_UP))
		activate (joysticks[port].hats[which].up);
	if (!(old & SDL_HAT_DOWN) && (value & SDL_HAT_DOWN))
		activate (joysticks[port].hats[which].down);
	if ((old & SDL_HAT_LEFT) && !(value & SDL_HAT_LEFT))
		deactivate (joysticks[port].hats[which].left);
	if ((old & SDL_HAT_RIGHT) && !(value & SDL_HAT_RIGHT))
		deactivate (joysticks[port].hats[which].right);
	if ((old & SDL_HAT_UP) && !(value & SDL_HAT_UP))
		deactivate (joysticks[port].hats[which].up);
	if ((old & SDL_HAT_DOWN) && !(value & SDL_HAT_DOWN))
		deactivate (joysticks[port].hats[which].down);
	joysticks[port].hats[which].last = value;
}

void
VControl_ResetInput ()
{
	/* Step through every valid entry in the binding pool and zero
	 * them out.  This will probably zero entries multiple times;
	 * oh well, no harm done. */

	keypool *base = pool;
	while (base != NULL)
	{
		int i;
		for (i = 0; i < POOL_CHUNK_SIZE; i++)
		{
			if(base->pool[i].target)
			{
				*(base->pool[i].target) = 0;
			}
		}
		base = base->next;
	}
}

void
VControl_HandleEvent (const SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:
			VControl_ProcessKeyDown (e->key.keysym.sym);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_KEYUP:
			VControl_ProcessKeyUp (e->key.keysym.sym);
			break;
		case SDL_JOYAXISMOTION:
			VControl_ProcessJoyAxis (e->jaxis.which, e->jaxis.axis, e->jaxis.value);
			break;
		case SDL_JOYHATMOTION:
			VControl_ProcessJoyHat (e->jhat.which, e->jhat.hat, e->jhat.value);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_JOYBUTTONDOWN:
			VControl_ProcessJoyButtonDown (e->jbutton.which, e->jbutton.button);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_JOYBUTTONUP:
			VControl_ProcessJoyButtonUp (e->jbutton.which, e->jbutton.button);
			break;
		default:
			break;
	}
}

void
VControl_RegisterNameTable (VControl_NameBinding *table)
{
	nametable = table;
}

static char *
target2name (int *target)
{
	VControl_NameBinding *b = nametable;
	while (b->target)
	{
		if (target == b->target)
		{
			return b->name;
		}
		++b;
	}
	return NULL;
}

static int *
name2target (char *name)
{
	VControl_NameBinding *b = nametable;
	while (b->target)
	{
		if (!stricmp (name, b->name))
		{
			return b->target;
		}
		++b;
	}
	return NULL;
}

static void
dump_keybindings (FILE *out, keybinding *kb, char *name)
{
	while (kb != NULL)
	{
		char *targetname = target2name (kb->target);
		fprintf (out, "%s: %s\n", targetname, name);
		kb = kb->next;
	}
}

void
VControl_Dump (FILE *out)
{
	int i;
	char namebuffer[64];

	/* Print out keyboard bindings */
	for (i = 0; i < SDLK_LAST; i++)
	{
		keybinding *kb = bindings[i];		
		if (kb != NULL)
		{
			sprintf (namebuffer, "key %s", VControl_code2name (i));
			dump_keybindings (out, kb, namebuffer);
		}
	}

	/* Print out joystick bindings */
	for (i = 0; i < joycount; i++)
	{
		if (joysticks[i].stick)
		{
			int j;

			fprintf (out, "joystick %d threshold %d\n", i, joysticks[i].threshold);
			for (j = 0; j < joysticks[i].numaxes; j++)
			{
				sprintf (namebuffer, "joystick %d axis %d negative", i, j);
				dump_keybindings (out, joysticks[i].axes[j].neg, namebuffer);
				sprintf (namebuffer, "joystick %d axis %d positive", i, j);
				dump_keybindings (out, joysticks[i].axes[j].pos, namebuffer);
			}
			for (j = 0; j < joysticks[i].numbuttons; j++)
			{
				keybinding *kb = joysticks[i].buttons[j];
				if (kb != NULL)
				{
					sprintf (namebuffer, "joystick %d button %d", i, j);
					dump_keybindings (out, kb, namebuffer);
				}
			}
			for (j = 0; j < joysticks[i].numhats; j++)
			{
				sprintf (namebuffer, "joystick %d hat %d left", i, j);
				dump_keybindings (out, joysticks[i].hats[j].left, namebuffer);
				sprintf (namebuffer, "joystick %d hat %d right", i, j);
				dump_keybindings (out, joysticks[i].hats[j].right, namebuffer);
				sprintf (namebuffer, "joystick %d hat %d up", i, j);
				dump_keybindings (out, joysticks[i].hats[j].up, namebuffer);
				sprintf (namebuffer, "joystick %d hat %d down", i, j);
				dump_keybindings (out, joysticks[i].hats[j].down, namebuffer);
			}
		}
	}

#ifdef VCONTROL_DEBUG
	/* Print out allocation data */
	{
		keypool bp = pool;
		i = 0;
		while (bp != NULL)
		{
			fprintf (out, "# Internal Debug: Chunk #%i: %d slots remaining.\n", i, bp->remaining);
			i++;
			bp = bp->next;
		}
		fprintf (out, "# Internal Debug: %d chunks allocated.\n", i);
	}
#endif
}

/* Iterator routines.  iter_target holds the target keybinding.
   iter_device is -1 for the keyboard, otherwise it is joystick
   #(iter_device/3).  (iter_device%3) gives whether we're checking the
   joystick's axes (0), buttons (1), or hats (2). */

static int *iter_target;
static int iter_device, iter_index;

void
VControl_StartIter (int *target)
{
	iter_target = target;
	iter_device = -1;
	iter_index = 0;
}

void
VControl_StartIterByName (char *targetname)
{
	VControl_StartIter (name2target (targetname));
}

int
VControl_NextBinding (VCONTROL_GESTURE *gesture)
{
	if ((gesture == NULL) || (iter_target == NULL)
			|| iter_device >= (joycount * 3))
		return 0;
	while (iter_device < (joycount * 3))
	{
		if (iter_device == -1) /* keyboard */
		{
			keybinding *kb = bindings[iter_index];
			int done = 0;
			while (kb != NULL) 
			{
				if (kb->target == iter_target)
				{
					gesture->type = VCONTROL_KEY;
					gesture->gesture.key = iter_index;
					done = 1;
				}
				kb = kb->next;
			}
			if (++iter_index == SDLK_LAST)
			{
				iter_device = 0;
				iter_index = 0;
			}
			if (done) return 1;
		}
		else
		{
			int i = iter_device / 3;
			int t = iter_device % 3;
			if (!joysticks[i].stick)  /* Joystick wasn't opened; this binding is dead */
			{
				iter_device++;
				iter_index = 0;
			}
			else
			{
				int done = 0;
				switch (t)
				{
				case 0: { /* Axes */
					int axis = iter_index / 2;
					int dir = iter_index % 2;
					if (axis >= joysticks[i].numaxes)
					{
						iter_device++;
						iter_index = 0;
					}
					else
					{
						keybinding *kb = (dir) ? joysticks[i].axes[axis].pos : joysticks[i].axes[axis].neg;
						while (kb != NULL) 
						{
							if (kb->target == iter_target)
							{
								done = 1;
								gesture->type = VCONTROL_JOYAXIS;
								gesture->gesture.axis.port = i;
								gesture->gesture.axis.index = axis;
								gesture->gesture.axis.polarity = (dir) ? 1 : -1;
							}
							kb = kb->next;
						}
						iter_index++;
					}
					break;
				}
				case 1: { /* Buttons */
					keybinding *kb = joysticks[i].buttons[iter_index];
					if (iter_index >= joysticks[i].numbuttons)
					{
						iter_device++;
						iter_index = 0;
					}
					else
					{
						while (kb != NULL) 
						{
							if (kb->target == iter_target)
							{
								gesture->type = VCONTROL_JOYBUTTON;
								gesture->gesture.button.port = i;
								gesture->gesture.button.index = iter_index;
								done = 1;
							}
							kb = kb->next;
						}
						iter_index++;
					}
					break;
				}
				case 2: { /* Hats */
					int hat = iter_index / 4;
					int hatd = iter_index % 4;
					if (hat >= joysticks[i].numhats)
					{
						iter_device++;
						iter_index = 0;
					}
					else
					{
						keybinding *kb;
						Uint8 targetdir;
						switch (hatd) 
						{
						case 0: kb = joysticks[i].hats[hat].left; targetdir = SDL_HAT_LEFT; break;
						case 1: kb = joysticks[i].hats[hat].right; targetdir = SDL_HAT_RIGHT; break;
						case 2: kb = joysticks[i].hats[hat].up; targetdir = SDL_HAT_UP; break;
						default: kb = joysticks[i].hats[hat].down; targetdir = SDL_HAT_DOWN; break;
						}
						while (kb != NULL) 
						{
							if (kb->target == iter_target)
							{
								done = 1;
								gesture->type = VCONTROL_JOYHAT;
								gesture->gesture.hat.port = i;
								gesture->gesture.hat.index = hat;
								gesture->gesture.hat.dir = targetdir;
							}
							kb = kb->next;
						}
						iter_index++;
					}
					break;
				}
				}
				if (done) return 1;
			}
		}
	}
	return 0;
}

/* Tracking the last interesting event */

void
VControl_ClearEvent (void)
{
	event_ready = 0;
}

int
VControl_GetLastEvent (SDL_Event *e)
{
	if (event_ready && e != NULL)
	{
		*e = last_interesting;
	}
	return event_ready;
}		

/* Configuration file grammar is as follows:  One command per line, 
 * hashes introduce comments that persist to end of line.  Blank lines
 * are ignored.
 *
 * Terminals are represented here as quoted strings, e.g. "foo" for 
 * the literal string foo.  These are matched case-insensitively.
 * Special terminals are:
 *
 * KEYNAME:  This names a key, as defined in keynames.c.
 * IDNAME:   This is an arbitrary string of alphanumerics, 
 *           case-insensitive, and ending with a colon.  This
 *           names an application-specific control value.
 * NUM:      This is an unsigned integer.
 * EOF:      End of file
 *
 * Nonterminals (the grammar itself) have the following productions:
 * 
 * configline <- IDNAME binding
 *             | "joystick" NUM "threshold" NUM
 *             | "version" NUM
 *
 * binding    <- "key" KEYNAME
 *             | "joystick" NUM joybinding
 *
 * joybinding <- "axis" NUM polarity
 *             | "button" NUM
 *             | "hat" NUM direction
 *
 * polarity   <- "positive" | "negative"
 *
 * dir        <- "up" | "down" | "left" | "right"
 *
 * This grammar is amenable to simple recursive descent parsing;
 * in fact, it's fully LL(1). */

/* Actual maximum line and token sizes are two less than this, since
 * we need space for the \n\0 at the end */
#define LINE_SIZE 256
#define TOKEN_SIZE 64

typedef struct vcontrol_parse_state {
	char line[LINE_SIZE];
	char token[TOKEN_SIZE];
	int index;
	int error;
	int linenum;
} parse_state;

static void
next_token (parse_state *state)
{
	int index, base;

	state->token[0] = 0;
	/* skip preceding whitespace */
	base = state->index;
	while (state->line[base] && isspace (state->line[base]))
	{
		base++;
	}

	index = 0;
	while (index < (TOKEN_SIZE-1) && state->line[base+index] && !isspace (state->line[base+index]))
	{
		state->token[index] = state->line[base+index];
		index++;
	}
	state->token[index] = 0;

	/* If the token was too long, skip ahead until we get to whitespace */
	while (state->line[base+index] && !isspace (state->line[base+index]))
	{
		index++;
	}

	state->index = base+index;
}

static void
next_line (parse_state *state, uio_Stream *in)
{
	int i, ch = 0;
	state->linenum++;
	for (i = 0; i < LINE_SIZE-1; i++)
	{
		if (uio_feof (in))
		{
			break;
		}
		ch = uio_fgetc (in);
		if (ch == '#' || ch == '\n' || ch == EOF)
		{
			/* If this line is blank or all commented, include some
			 * whitespace.  This lets us detect EOF as a completely
			 * blank line. */
			if (i==0) 
			{
				state->line[i] = '\n';
				i++;
			} 
			break;
		}
		state->line[i] = ch;
	}
	state->line[i] = '\0';
	/* Skip to end of line */
	while (ch != '\n' && !uio_feof (in))
	{
		ch = uio_fgetc (in);
	}
	state->token[0] = 0;
	state->index = 0;
	state->error = 0;
}

static void
expected_error (parse_state *state, char *expected)
{
	fprintf (stderr, "VControl: Expected '%s' on config file line %d\n", expected, state->linenum);
	state->error = 1;
}

static void
consume (parse_state *state, char *expected)
{
	if (stricmp (expected, state->token))
	{
		expected_error (state, expected);
	}
	next_token (state);
}

static int
consume_keyname (parse_state *state)
{
	int keysym = VControl_name2code (state->token);
	if (!keysym)
	{
		fprintf (stderr, "VControl: Illegal key name '%s' on config file line %d\n", state->token, state->linenum);
		state->error = 1;
	}
	next_token (state);
	return keysym;
}

static int *
consume_idname (parse_state *state)
{
	int *result = NULL;
	int index = 0;
	while (state->token[index]) 
	{
		index++;
	}

	if (index == 0)
	{
		fprintf (stderr, "VControl: Can't happen: blank token to consume_idname (line %d)\n", state->linenum);
		state->error = 1;
		return NULL;
	}

	index--;
	if (state->token[index] != ':')
	{
		expected_error (state, ":");
		return NULL;
	}

	state->token[index] = 0;  /* remove trailing colon */

	result = name2target (state->token);

	if (!result)
	{
		fprintf (stderr, "VControl: Illegal command type '%s' on config file line %d\n", state->token, state->linenum);
		state->error = 1;
	}
	next_token (state);
	return result;
}

static int
consume_num (parse_state *state)
{
	char *end;
	int result = strtol (state->token, &end, 10);
	if (*end != '\0')
	{
		fprintf (stderr, "VControl: Expected integer on config line %d\n", state->linenum);
		state->error = 1;
	}
	next_token (state);
	return result;
}

static int
consume_polarity (parse_state *state)
{
	int result = 0;
	if (!stricmp (state->token, "positive"))
	{
		result = 1;
	}
	else if (!stricmp (state->token, "negative"))
	{
		result = -1;
	}
	else
	{
		expected_error (state, "positive' or 'negative");
	}
	next_token (state);
	return result;
}

static Uint8
consume_dir (parse_state *state)
{
	Uint8 result = 0;
	if (!stricmp (state->token, "left"))
	{
		result = SDL_HAT_LEFT;
	}
	else if (!stricmp (state->token, "right"))
	{
		result = SDL_HAT_RIGHT;
	}
	else if (!stricmp (state->token, "up"))
	{
		result = SDL_HAT_UP;
	}
	else if (!stricmp (state->token, "down"))
	{
		result = SDL_HAT_DOWN;
	}
	else
	{
		expected_error (state, "left', 'right', 'up' or 'down");
	}
	next_token (state);
	return result;
}

static void
parse_joybinding (parse_state *state, int *target)
{
	int sticknum;
	consume (state, "joystick");
	sticknum = consume_num (state);
	if (!state->error)
	{
		if (!stricmp (state->token, "axis"))
		{
			int axisnum;
			consume (state, "axis");
			axisnum = consume_num (state);
			if (!state->error)
			{
				int polarity = consume_polarity (state);
				if (!state->error)
				{
					if (VControl_AddJoyAxisBinding (sticknum, axisnum, polarity, target))
					{
						// Don't count this as an error
						// state->error = 1;
					}
				}
			}
		} 
		else if (!stricmp (state->token, "button"))
		{
			int buttonnum;
			consume (state, "button");
			buttonnum = consume_num (state);
			if (!state->error)
			{
				if (VControl_AddJoyButtonBinding (sticknum, buttonnum, target))
				{
					// Don't count this as an error
					// state->error = 1;
				}
			}
		}
		else if (!stricmp (state->token, "hat"))
		{
			int hatnum;
			consume (state, "hat");
			hatnum = consume_num (state);
			if (!state->error)
			{
				Uint8 dir = consume_dir (state);
				if (!state->error)
				{
					if (VControl_AddJoyHatBinding (sticknum, hatnum, dir, target))
					{
						// Don't count this as an error
						// state->error = 1;
					}
				}
			}
		}
		else
		{
			expected_error (state, "axis', 'button', or 'hat");
		}
	}
}

static void
parse_binding (parse_state *state)
{
	int *target = consume_idname (state);
	if (!state->error)
	{
		if (!stricmp (state->token, "key"))
		{
			/* Parse key binding */
			int keysym;
			consume (state, "key");
			keysym = consume_keyname (state);
			if (!state->error)
			{
				if (VControl_AddKeyBinding (keysym, target))
				{
					state->error = 1;
				}
			}
		}
		else if (!stricmp (state->token, "joystick"))
		{
			parse_joybinding (state, target);
		}
		else
		{
			expected_error (state, "key' or 'joystick");
		}
	}
}

static void
parse_config_line (parse_state *state)
{
	state->error = 0;
	next_token (state);
	if (!state->token[0])
	{
		/* Blank line, skip it */
		return;
	}
	if (!stricmp (state->token, "joystick"))
	{
		int sticknum, threshold = 0;
		consume (state, "joystick");
		sticknum = consume_num (state);
		if (!state->error) consume (state, "threshold");
		if (!state->error) threshold = consume_num (state);
		if (!state->error)
		{
			if (VControl_SetJoyThreshold (sticknum, threshold))
			{
				// Don't count this as an error
				// state->error = 1;
			}
		}
		if (!state->error)
		{
			validlines++;
		}
		return;
	}
	if (!stricmp (state->token, "version"))
	{
		consume (state, "version");
		version = consume_num (state);
		if (!state->error)
		{
			validlines++;
		}
		return;
	}
	/* Otherwise, it must be a binding */
	parse_binding (state);
	if (!state->error)
	{
		validlines++;
	}
}

int
VControl_ReadConfiguration (uio_Stream *in)
{
	parse_state ps;
	if (!in)
	{
		fprintf (stderr, "VControl: Invalid configuration file stream\n");
		return 1;
	}
	ps.linenum = 0;
	errors = version = validlines = 0;
	while (1)
	{
		next_line (&ps, in);
		if (!ps.line[0])
			break;
		parse_config_line (&ps);
		if (ps.error)
		{
			errors++;
		}
	}
	return errors;
}

int
VControl_GetErrorCount (void)
{
	return errors;
}

int
VControl_GetValidCount (void)
{
	return validlines;
}

int
VControl_GetConfigFileVersion (void)
{
	return version;
}

void
VControl_SetConfigFileVersion (int v)
{
	version = v;
}

#if 0
/* This was kinda handy for proving (lack of) buffer overrun
 * vulnerabilities, but there's no real need for it otherwise. */
void
VControl_TokenizeFile (FILE *in)
{
	parse_state ps;
	if (!in)
	{
		fprintf (stderr, "VControl: Invalid configuration file stream\n");
		return;
	}
	ps.linenum = 0;
	while (1)
	{
		_next_line (&ps, in);
		if (!ps.line[0])
			break;
		printf ("%3d:", ps.linenum);
		while (1)
		{
			_next_token (&ps);
			if (!ps.token[0])
				break;
			printf (" \"%s\"", ps.token);
		}
		printf ("\n");
	}
}
#endif
