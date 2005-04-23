/*	SDLMain.m - main entry point for our Cocoa-ized SDL app
	Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
	Non-NIB-Code & other changes: Max Horn <max@quendi.de>

	Feel free to customize this file to suit your needs

	Modified for use with The Ur-Quan Masters by Nicolas Simonds
	<uqm at submedia dot net>
*/

#import "port.h"
#import SDL_INCLUDE(SDL.h)
#import "SDLMain.h"
#import <sys/param.h>
		/* for PATH_MAX */
#import <string.h>
		/* for strrchr() */
#import <unistd.h>

static int gArgc;
static char **gArgv;
static BOOL gFinderLaunch;

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
	/* Post a SDL_QUIT event */
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent (&event);
	(void) sender;  /* Get rid of unused variable warning */
}

/* override NSApplication:sendEvent, to keep Cocoa from beeping on
   non-command keystrokes */
- (void)sendEvent:(NSEvent *)anEvent {
	if (NSKeyDown == [anEvent type] || NSKeyUp == [anEvent type]) {
		if ([anEvent modifierFlags] & NSCommandKeyMask)
			[super sendEvent: anEvent];
	} else
		[super sendEvent: anEvent];
}
@end

/* The main class of the application, the application's delegate */
@implementation SDLMain

static char *
basename (char *path)
{
	char *base;

	base = strrchr (path, '/');
	if (base == NULL)
		return path;

	return (base + 1);
}

/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
	char origindir[PATH_MAX];
	char *c;

	if (!shouldChdir)
		return;

	strncpy (origindir, gArgv[0], sizeof origindir);
	origindir[sizeof origindir - 1] = '\0';

	c = basename (origindir);
	if (c == origindir)
		strcpy (origindir, ".");
	else
		*c = '\0';

	/* chdir to the binary app's point of origin */
	if (chdir (origindir) != 0)
		abort ();
	/* then chdir to the .app's parent */
	if ( chdir ("../Resources/") != 0 )
		abort();
}

void
setupAppleMenu (void)
{
	NSMenu *appleMenu;

	NSMenuItem *menuItem;
	NSString *title;
	NSString *appName;

	appName = [NSString stringWithUTF8String:basename (gArgv[0])];
	appleMenu = [[NSMenu alloc] initWithTitle:appName];

	/* Add menu items */
	title = [@"Hide " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(hide:)
			keyEquivalent:@"h"];

	menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others"
			action:@selector(hideOtherApplications:)
			keyEquivalent:@"h"];
	[menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

	[appleMenu addItemWithTitle:@"Show All"
			action:@selector(unhideAllApplications:)
			keyEquivalent:@""];

	[appleMenu addItem:[NSMenuItem separatorItem]];

	title = [@"Quit " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(terminate:)
			keyEquivalent:@"q"];

	/* Put menu into the menubar */
	menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil
			keyEquivalent:@""];
	[menuItem setSubmenu:appleMenu];
	[[NSApp mainMenu] addItem:menuItem];

	/* Tell the application object that this is now the application menu */
	[NSApp setAppleMenu:appleMenu];

	/* Finally give up our references to the objects */
	[appleMenu release];
	[menuItem release];
}

/* Create a window menu */
void
setupWindowMenu (void)
{
	NSMenu *windowMenu;
	NSMenuItem *windowMenuItem;
	NSMenuItem *menuItem;

	windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

	/* "Minimize" item */
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize"
			action:@selector(performMiniaturize:)
			keyEquivalent:@"m"];
	[windowMenu addItem:menuItem];
	[menuItem release];

	/* Put menu into the menubar */
	windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window"
			action:nil keyEquivalent:@""];
	[windowMenuItem setSubmenu:windowMenu];
	[[NSApp mainMenu] addItem:windowMenuItem];

	/* Tell the application object that this is now the window menu */
	[NSApp setWindowsMenu:windowMenu];

	/* Finally give up our references to the objects */
	[windowMenu release];
	[windowMenuItem release];
}

/* Replacement for NSApplicationMain */
void
CustomApplicationMain (int argc, char **argv)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SDLMain *sdlMain;

	/* Ensure the application object is initialised */
	[SDLApplication sharedApplication];

	/* Set up the menubar */
	[NSApp setMainMenu:[[NSMenu alloc] init]];
	setupAppleMenu ();
	setupWindowMenu ();

	/* Create SDLMain and make it the app delegate */
	sdlMain = [[SDLMain alloc] init];
	[NSApp setDelegate:sdlMain];

	/* Start the main event loop */
	[NSApp run];

	[sdlMain release];
	[pool release];

	(void) argc;  /* Get rid of unused variable warning */
	(void) argv;  /* Get rid of unused variable warning */
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	int status;

	/* Set the working directory to the .app's parent directory */
	[self setupWorkingDirectory:gFinderLaunch];

	/* allow Cocoa to hear keystrokes like Command-Q, etc. */
	setenv ("SDL_ENABLEAPPEVENTS", "1", 1);

	/* Hand off to main application code */
	status = SDL_main (gArgc, gArgv);

	/* We're done, thank you for playing */
	exit (status);

	(void) note;  /* Get rid of unused variable warning */
}
@end

@implementation NSString (ReplaceSubString)

- (NSString *) stringByReplacingRange:(NSRange)aRange with:(NSString *)aString
{
	unsigned int bufferSize;
	unsigned int selfLen = [self length];
	unsigned int aStringLen = [aString length];
	unichar *buffer;
	NSRange localRange;
	NSString *result;

	bufferSize = selfLen + aStringLen - aRange.length;
	buffer = NSAllocateMemoryPages (bufferSize * sizeof (unichar));

	/* Get first part into buffer */
	localRange.location = 0;
	localRange.length = aRange.location;
	[self getCharacters:buffer range:localRange];

	/* Get middle part into buffer */
	localRange.location = 0;
	localRange.length = aStringLen;
	[aString getCharacters:(buffer + aRange.location) range:localRange];

	/* Get last part into buffer */
	localRange.location = aRange.location + aRange.length;
	localRange.length = selfLen - localRange.location;
	[self getCharacters:(buffer + aRange.location+aStringLen)
			range:localRange];

	/* Build output string */
	result = [NSString stringWithCharacters:buffer length:bufferSize];

	NSDeallocateMemoryPages (buffer, bufferSize);

	return result;
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int
main (int argc, char **argv)
{
	/* Copy the arguments into a global variable */
	int i;

	/* If we are launched by double-clicking, argv[1] is "-psn_<some_number> */
	if ( argc >= 2 && strncmp (argv[1], "-psn_", 5) == 0 ) {
		gArgc = 1;
		gFinderLaunch = YES;
	} else {
		gArgc = argc;
		gFinderLaunch = NO;
	}
	gArgv = (char **) malloc (sizeof *gArgv * (gArgc + 1));
	if (gArgv == NULL)
		abort ();
	for (i = 0; i < gArgc; i++)
		gArgv[i] = argv[i];
	gArgv[i] = NULL;

	CustomApplicationMain (argc, argv);
	free (gArgv);
	return 0;
}


