//
//  CelestiaController.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#include <unistd.h>
#import "CelestiaController.h"
#import "FavoritesDrawerController.h"
#import "CelestiaOpenGLView.h"
#import "FullScreenWindow.h"
#import "SplashScreen.h"
#import "SplashWindowController.h"
#import "EclipseFinderController.h"
#import "ScriptsController.h"
#import <Carbon/Carbon.h>
#import <OpenGL/gl.h>
#import "CGLInfo.h"

#include <float.h>


@implementation CelestiaController

static CelestiaController* firstInstance;

+(CelestiaController*) shared
{
    // class method to get single shared instance
    return firstInstance;
}

// Startup Methods ----------------------------------------------------------

NSString* fatalErrorMessage;

- (void)awakeFromNib
{
    if ([[self superclass] instancesRespondToSelector:@selector(awakeFromNib)]) 
    {
        [super awakeFromNib];
    }

    if (firstInstance == nil ) firstInstance = self;
    int cpuCount = 0;
    size_t cpuCountSize = sizeof cpuCount;
    if (0 == sysctlbyname("hw.ncpu", &cpuCount, &cpuCountSize, NULL, 0))
    {
        threaded = (cpuCount > 1);
    }
    ready = NO;
    isDirty = YES;
    isFullScreen = NO;
    appCore = nil;
    fatalErrorMessage = nil;
    lastScript = nil;

    [self setupResourceDirectory];
    [scriptsController buildScriptMenu];

    //  hide main window until ready
    [[glView window] setAlphaValue: 0.0f];  //  not  [[glView window] orderOut: nil];

    // create appCore
    appCore = [CelestiaAppCore sharedAppCore];
    // check for startup failure
    if (appCore == nil)
    {
        NSLog(@"Could not create CelestiaAppCore!");
        [NSApp terminate:self];
        return;
    }
    
	BOOL nosplash = [[NSUserDefaults standardUserDefaults] boolForKey:@"nosplash"];
    startupCondition = [[NSConditionLock alloc] initWithCondition: 0];

	if (!nosplash)
	{
		[splashWindowController showWindow];
	}
    if (threaded)
    {
        // start initialization thread
        [NSThread detachNewThreadSelector: @selector(startInitialization) toTarget: self
        withObject: nil];
        // wait for completion
        [self performSelectorOnMainThread: @selector(waitWhileLoading:) withObject: nil waitUntilDone: NO ];
    }
    else
    {
        [self performSelector:@selector(startInitialization) withObject:nil afterDelay:0];
    }
}


- (void) setupResourceDirectory
{
    NSBundle* mainBundle = [NSBundle mainBundle];
    // Change directory to resource dir so Celestia can find cfg files and textures
    NSFileManager *fileManager = [NSFileManager defaultManager]; 
    NSString* path;
    NSMutableArray *resourceDirs = [NSMutableArray array];
    BOOL isFolder = NO;

    if ( [ fileManager fileExistsAtPath: path = [[ mainBundle resourcePath ] stringByAppendingPathComponent: CELESTIA_RESOURCES_FOLDER ] isDirectory: &isFolder ] && isFolder )
    {
        [resourceDirs addObject: path];
    }
    if ( [ fileManager fileExistsAtPath: path = [[[ mainBundle bundlePath ]  stringByDeletingLastPathComponent] stringByAppendingPathComponent: CELESTIA_RESOURCES_FOLDER ] isDirectory: &isFolder ] && isFolder )
    {
        [resourceDirs addObject: path];
    }
    FSRef folder;
    CFURLRef url;
    static short domains[] = { kUserDomain, kLocalDomain, kNetworkDomain };
    unsigned i;
    path = nil;

    for (i = 0; i < (sizeof domains / sizeof(short)); ++i)
    {
        if (FSFindFolder(domains[i], kApplicationSupportFolderType, FALSE, &folder) == noErr)
        {
            url = CFURLCreateFromFSRef(nil, &folder);
            path = [(NSURL *)url path];
            CFRelease(url);

            if (path)
            {
                if (![fileManager fileExistsAtPath: path = [path stringByAppendingPathComponent: CELESTIA_RESOURCES_FOLDER]] &&
                    kUserDomain==domains[i])
                {
                    if ([fileManager createDirectoryAtPath:path attributes:nil])
                    {
                        [fileManager createDirectoryAtPath:[path stringByAppendingPathComponent:@"extras"] attributes:nil];
                        [fileManager createDirectoryAtPath:[path stringByAppendingPathComponent:CEL_SCRIPTS_FOLDER] attributes:nil];
                    }
                }
                if ([fileManager fileExistsAtPath:path isDirectory:&isFolder] && isFolder)
                {
                    [resourceDirs addObject: path];
                }
            }

            path = nil;
        }
    }

    if ([resourceDirs count] > 0)
    {
        [fileManager changeCurrentDirectoryPath: [resourceDirs objectAtIndex: 0]];
        [resourceDirs removeObjectAtIndex: 0];
        if ([resourceDirs count] > 0) {
            NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
            [prefs registerDefaults:[NSDictionary dictionaryWithObject:[NSArray arrayWithObject:[resourceDirs objectAtIndex:0]] forKey:@"existingResourceDirs"]];
        }
    }
    else 
    {
        [self fatalError: NSLocalizedString(@"It appears that the \"CelestiaResources\" directory has not been properly installed in the correct location as indicated in the installation instructions. \n\nPlease correct this and try again.",@"")];
        [self fatalError: nil];
    }
}

- (void)startInitialization
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    [[glView openGLContext] makeCurrentContext];
#ifdef DEBUG
    NSDate *t = [NSDate date];
#endif
    if (![appCore initSimulation])
    {
        [startupCondition lock];
        [startupCondition unlockWithCondition: 99];
#ifdef DEBUG
        [pool release];
#endif
        [self fatalError: NSLocalizedString(@"Error loading data files. Celestia will now quit.",@"")];
        if (!threaded)
            [self fatalError: nil];
        else
            [NSThread exit];
        return;
    }

#ifdef DEBUG
    NSLog(@"Init took %lf seconds\n", -[t timeIntervalSinceNow]);
#endif
    [startupCondition lock];
    [startupCondition unlockWithCondition: 1];
    if (!threaded)
    {
        [splashWindowController close];
        // complete startup
        [self fatalError: nil];
        [self finishInitialization];
    }

    [pool release];
}


- (void) fatalError: (NSString *) msg
{
    // handle fatal error message from either main or loading threads
    if ( msg == nil )
    {
        if (fatalErrorMessage == nil) return;
        [splashWindowController close];
        NSRunAlertPanel(NSLocalizedString(@"Fatal Error",@""), fatalErrorMessage, nil, nil, nil);
        fatalErrorMessage = nil;    // user could cancel the terminate
        [NSApp terminate:self];
        return;
    }
    fatalErrorMessage = [msg retain];
}

- (void) waitWhileLoading: (id) obj
{
    // display loading indicator window while loading

    static NSModalSession session = nil;

    if ( [startupCondition condition] == 0 ) 
    {
        if ( session != nil )
            return;
        // beginModalSession also displays the window, but the centering
        // is wrong so do the display and centering beforehand
        session = [NSApp beginModalSessionForWindow: [splashWindowController window]];
        for (;;) 
        {
            if ( fatalErrorMessage != nil )
                break;
            if ([NSApp runModalSession:session] != NSRunContinuesResponse)
                break;
            if ( [startupCondition condition] != 0 )
                break;
        }
        [NSApp endModalSession:session];
    }
    [splashWindowController close];
    // check for fatal error in loading thread
    [self fatalError: nil];
    // complete startup
    [self finishInitialization];
}

-(void) setupFavorites
{
    NSInvocation *menuCallback;

    menuCallback = [NSInvocation invocationWithMethodSignature:[FavoritesDrawerController instanceMethodSignatureForSelector:@selector(synchronizeFavoritesMenu)]];
    [menuCallback setSelector:@selector(synchronizeFavoritesMenu)];
    [menuCallback setTarget:favoritesDrawerController];
    [[CelestiaFavorites sharedFavorites] setSynchronize:menuCallback];
    [[CelestiaFavorites sharedFavorites] synchronize];
}

-(void) startGLView
{
    [[glView window] setAutodisplay:YES];
    [[glView window] setHidesOnDeactivate: NO];
    [[glView window] setFrameUsingName: @"Celestia"];
    [[glView window] setAlphaValue: 1.0f];
    [[glView window] setFrameAutosaveName: @"Celestia"];
    if ([[glView window] canBecomeMainWindow])
        [[glView window] makeMainWindow ];
    [[glView window] makeFirstResponder: glView ];
    [[glView window] makeKeyAndOrderFront: glView ];
    [glView registerForDraggedTypes:
        [NSArray arrayWithObjects: NSStringPboardType, NSFilenamesPboardType, NSURLPboardType, nil]];
    [glView setNeedsDisplay:YES];
}

- (void)startRender
{
    [self startGLView];

    if (isFullScreen)
    {
        isFullScreen = NO;
        [self toggleFullScreen: self];
    }

    // workaround for fov problem
    if (pendingUrl) [appCore goToUrl: pendingUrl];

    if ([startupCondition tryLockWhenCondition: 1])
        [startupCondition unlockWithCondition:  2];
}

- (void)finishInitialization
{
#ifndef NO_VP_WORKAROUND
    NSString *VP_PROBLEM_EXT  = @"GL_ARB_vertex_program";
    NSString *VP_PATCH_SCRIPT = @"vp_patch.sh";
    NSString *VP_PATCH_SHELL  = @"/bin/zsh";
    NSString *CELESTIA_CFG    = @"~/.celestia.cfg";

    const char *VP_PROBLEM_RENDERERS[] = { "ATI Radeon 9200" };
    const char *glRenderer = (const char *) glGetString(GL_RENDERER);
    BOOL shouldWorkaround = NO;
    size_t i = 0;

    if (glRenderer)
    {
        for (; i < (sizeof VP_PROBLEM_RENDERERS)/sizeof(char *); ++i)
        {
            if (strstr(glRenderer, VP_PROBLEM_RENDERERS[i]))
            {
                shouldWorkaround = YES;
                break;
            }
        }
    }

    if (shouldWorkaround && ![appCore glExtensionIgnored: VP_PROBLEM_EXT])
    {
        if (NSRunAlertPanel([NSString stringWithFormat: NSLocalizedString(@"It appears you are running Celestia on %s hardware. Do you wish to install a workaround?",nil), VP_PROBLEM_RENDERERS[i]],
                            [NSString stringWithFormat: NSLocalizedString(@"A shell script will be run to modify your %@, adding an IgnoreGLExtensions directive. This can prevent freezing issues.",nil), CELESTIA_CFG],
                            NSLocalizedString(@"Yes",nil),
                            NSLocalizedString(@"No",nil),
                            nil) == NSAlertDefaultReturn)
        {
            // Install it
            NSString *cfgPath = [CELESTIA_CFG stringByStandardizingPath];
            NSString *toolPath = [[NSBundle mainBundle] pathForResource: VP_PATCH_SCRIPT ofType: @""];
            BOOL patchInstalled = NO;

            if (toolPath)
            {
                NSArray *taskArgs = [NSArray arrayWithObjects:
                    toolPath, cfgPath, nil];
                NSTask *theTask = [NSTask launchedTaskWithLaunchPath: VP_PATCH_SHELL
                                                           arguments: taskArgs];
                if (theTask)
                {
                    [theTask waitUntilExit];
                    patchInstalled = ([theTask terminationStatus] == 0);
                }
            }

            if (patchInstalled)
            {
                // Have to apply same patch to config already loaded in memory
                [appCore setGLExtensionIgnored: VP_PROBLEM_EXT];
                NSRunAlertPanel(NSLocalizedString(@"Workaround successfully installed.",nil),
                                [NSString stringWithFormat: NSLocalizedString(@"Your original %@ has been backed up.",nil), CELESTIA_CFG],
                                nil, nil, nil);
            }
            else
            {
                [[CelestiaController shared] fatalError: NSLocalizedString(@"There was a problem installing the workaround. You can attempt to perform the workaround manually by following the instructions in the README.",nil)];
                [[CelestiaController shared] fatalError: nil];
            }
        }
    }
#endif NO_VP_WORKAROUND

    [glView setAASamples: [appCore aaSamples]];
    [appCore initRenderer];

    [self setupFavorites];

    settings = [CelestiaSettings shared];
    [settings setControl: self];
    [settings scanForKeys: [renderPanelController window]];
    [settings validateItems];

    // paste URL if pending
    if (pendingUrl != nil )
    {
        [ appCore setStartURL: pendingUrl ];
    }

    // Settings used to be loaded after starting simulation due to
    // timezone setting requiring simulation time, but this dependency
    // has been removed. In fact timezone needs to be set in order to
    // correctly set the simulation time so settings loaded before starting.
    [settings loadUserDefaults];

    [appCore start:[NSDate date]];

    ready = YES;
    timer = [[NSTimer timerWithTimeInterval: 0.01 target: self selector:@selector(timeDisplay) userInfo:nil repeats:YES] retain];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];

    // Threaded startup can allow app to be hidden during startup
    // When this happens delay rendering until first unhide
    // to prevent all sorts of problems
    if (![NSApp isHidden])
        [self startRender];

    // run script if pending (scripts can run without rendering)
    if (pendingScript != nil )
    {
        [self runScript: pendingScript ];
    }    
}

// Application Event Handler Methods ----------------------------------------------------------

- (void) applicationWillFinishLaunching:(NSNotification *) notification {
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self andSelector:@selector( handleURLEvent:withReplyEvent: ) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (BOOL) application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    if ( ready )
        [self runScript: filename ];
    else
        pendingScript = [filename retain];
    return YES;
}

- (void) handleURLEvent:(NSAppleEventDescriptor *) event withReplyEvent:(NSAppleEventDescriptor *) replyEvent 
{
    if ( ready )
        [ appCore goToUrl: [[event descriptorAtIndex:1] stringValue] ];
    else
        pendingUrl = [[[event descriptorAtIndex:1] stringValue] retain];
}

/* On a multi-screen setup, user is able to change the resolution of the screen running Celestia from a different screen, or the menu bar position so handle that */
- (void)applicationDidChangeScreenParameters:(NSNotification *) aNotification
{
    if (isFullScreen && aNotification && ([aNotification object] == NSApp))
    {
        // If menu bar not on same screen, don't hide it anymore
        if (![self hideMenuBarOnActiveScreen])
            SetSystemUIMode(kUIModeNormal, 0);

        NSScreen *screen = [[self window] screen];
        NSRect screenRect = [screen frame];

        if (!NSEqualSizes(screenRect.size, [[self window] frame].size))
            [[self window] setFrame: screenRect display:YES];
    }
}

- (void)applicationDidHide:(NSNotification *)aNotification
{
    ready = NO;
}

- (void)applicationWillUnhide:(NSNotification *)aNotification
{
    if ( [startupCondition condition] == 0 ) return;
    ready = YES;
}

- (void)applicationDidUnhide:(NSNotification *)aNotification
{
    if ( [startupCondition tryLockWhenCondition: 1] )
    {
        [startupCondition unlock];
        [self startRender];
    }
}

-(BOOL)applicationShouldTerminate:(id)sender
{
   if (  NSRunAlertPanel(NSLocalizedString(@"Quit Celestia?",@""),
                         NSLocalizedString(@"Are you sure you want to quit Celestia?",@""),
                         NSLocalizedString(@"Quit",@""),
                         NSLocalizedString(@"Cancel",@""),
                         nil) != NSAlertDefaultReturn ) 
   {
       return NO;
   }

    if (timer != nil) {
        [timer invalidate];
        [timer release];
        timer = nil;
    }
    [[CelestiaAppCore sharedAppCore] archive];
    return YES;
}

- (void) applicationWillTerminate:(NSNotification *) notification
{
    [settings storeUserDefaults];

    [lastScript release];
    [eclipseFinderController release];
    [browserWindowController release];
    [helpWindowController release];

    if (appCore != nil) {
        [appCore release];
        appCore = nil;
    }
}


// Window Event Handler Methods ----------------------------------------------------------

-(BOOL)windowShouldClose:(id)sender
{
    [NSApp terminate:nil];
    return NO;
}

- (void)resize 
{
    [appCore resize:[glView frame]];
    isDirty = NO;
}

// Catch key events even when gl window is not key
-(void)delegateKeyDown:(NSEvent *)theEvent
{
    if (ready)
        [glView keyDown: theEvent];
}

// Held Key Simulation Methods ----------------------------------------------------------

-(void) keyPress:(int) code hold: (int) time
{
    // start simulated key hold
    keyCode = code;
    keyTime = time;
    [appCore keyDown: keyCode ];
}

- (void) keyTick
{
       if ( keyCode != 0 )
       {
            if ( keyTime <= 0 )
            {
               [ appCore keyUp: keyCode];
               keyCode = 0;  
            }
            else 
               keyTime --;
       }    
}

// Display Update Management Methods ----------------------------------------------------------

- (void)setDirty
{
    isDirty = YES;
}

- (void) forceDisplay
{
    if (![glView needsDisplay]) [glView setNeedsDisplay:YES];
}

- (void) display 
{
    // update display when required by glView (invoked from drawRect:)
    if (ready)
    {
        if (isDirty)
            [self resize];
        // render to glView
        [appCore draw];
        // update scene
        [appCore tick];
    }
}

- (void) timeDisplay
{
//    if (!ready) return;

    [NSEvent stopPeriodicEvents];    

    // check for time to release simulated key held down
    [self keyTick];
#ifdef USE_PERIODIC
    // adjust timer if necessary to receive waiting appkit events
    static NSEvent* lastEvent = nil;
    NSEvent *nextEvent;

    [NSEvent startPeriodicEventsAfterDelay: 0.0 withPeriod: 0.001 ];
    nextEvent = [NSApp nextEventMatchingMask: ( NSPeriodicMask|NSAppKitDefinedMask ) untilDate: nil inMode: NSDefaultRunLoopMode dequeue: NO];
    [NSEvent stopPeriodicEvents]; 

    if ( [nextEvent type] == NSPeriodic )
    {   
        // ignore periodic events
        [NSApp discardEventsMatchingMask: NSPeriodicMask beforeEvent: nil ];
    }
    else
    {
        if ( nextEvent == lastEvent )
    {   
        // event is still waiting, so delay firing timer to allow event to process
        [timer setFireDate: [[NSDate date] addTimeInterval: 0.01 ] ];
    }
    else
    {
        lastEvent = nextEvent;
            return;
    }
    }
#endif
    // force display update
    [self forceDisplay];
}

// Application Action Methods ----------------------------------------------------------


/* Full screen toggle method. Uses a borderless window that covers the screen so that context menus continue to work. */
- (IBAction) toggleFullScreen: (id) sender
{
    if (isFullScreen)
    {
        CelestiaOpenGLView *windowedView = nil;
        Class viewClass = [CelestiaOpenGLView class];
        NSArray *mainSubViews = [[origWindow contentView] subviews];

        if (mainSubViews && [mainSubViews count]>0)
        {
            // Just to be safe, search for first child of correct type
            NSEnumerator *viewEnum = [mainSubViews objectEnumerator];
            id subView;
            while ((subView = [viewEnum nextObject]))
            {
                if ([subView isKindOfClass: viewClass])
                {
                    windowedView = subView;
                    break;
                }
            }
        }
        else if ([[origWindow contentView] isKindOfClass: viewClass])
            windowedView = [origWindow contentView];

        [origWindow makeKeyAndOrderFront: self];

        if (windowedView == nil)
        {
            // Can't switch back to windowed mode, but hide full screen window
            // so user can still quit the program
            [[self window] orderOut: self];
            SetSystemUIMode(kUIModeNormal, 0);
            [self fatalError: NSLocalizedString(@"Unable to properly exit full screen mode. Celestia will now quit.",@"")];
            [self performSelector:@selector(fatalError:) withObject:nil afterDelay:0.1];
            return;
        }

        [windowedView setOpenGLContext: [glView openGLContext]];
        [[glView openGLContext] setView: windowedView];

        [[self window] close];  // full screen window releases on close
        SetSystemUIMode(kUIModeNormal, 0);
        [self setWindow: origWindow];
        glView = windowedView;
        [self setDirty];
        [origWindow makeMainWindow];
        isFullScreen = NO;
        return;
    }

    // We will take over the screen that the window is on
    // (if there are >1 screens, the 50% rule applies)
    NSScreen *screen = [[glView window] screen];

    CelestiaOpenGLView *fullScreenView = [[CelestiaOpenGLView alloc] initWithFrame:[glView frame] pixelFormat:[glView pixelFormat]];
    [fullScreenView setMenu: [glView menu]];    // context menu

    FullScreenWindow *fullScreenWindow = [[FullScreenWindow alloc] initWithScreen: screen];
    [fullScreenWindow fadeOutScreen];

    [fullScreenWindow setBackgroundColor: [NSColor blackColor]];
    [fullScreenWindow setReleasedWhenClosed: YES];
    [self setWindow: fullScreenWindow]; // retains it
    [fullScreenWindow release];
    [fullScreenWindow setDelegate: self];
    // Hide the menu bar only if it's on the same screen
    [self hideMenuBarOnActiveScreen];
    [fullScreenWindow makeKeyAndOrderFront: nil];

    [fullScreenWindow setContentView: fullScreenView];
    [fullScreenView release];
    [fullScreenView setOpenGLContext: [glView openGLContext]];
    [[glView openGLContext] setView: fullScreenView];

    // Remember the original (bordered) window
    origWindow = [glView window];
    // Close the original window (does not release it)
    [origWindow close];
    glView = fullScreenView;
    [glView takeValue: self forKey: @"controller"];
    [fullScreenWindow makeFirstResponder: glView];
    // Make sure the view looks ready before unfading from black
    [glView update];
    [glView display];

    [fullScreenWindow makeMainWindow];
    [fullScreenWindow restoreScreen];
    isFullScreen = YES;
}

- (BOOL) hideMenuBarOnActiveScreen
{
    NSScreen *screen = [[self window] screen];
    NSArray *allScreens = [NSScreen screens];
    if (allScreens && [allScreens objectAtIndex: 0]!=screen)
        return NO;

    SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
    return YES;
}

- (void) runScript: (NSString*) path
{
    NSString* oldScript = lastScript;
    lastScript = [path retain];
    [oldScript release];
    [appCore runScript: lastScript];       
}

- (IBAction) openScript: (id) sender
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    NSDocumentController *dc = [NSDocumentController sharedDocumentController];
    int result = [panel runModalForTypes: [dc fileExtensionsFromType:@"Celestia Script"]];
    if (result == NSOKButton)
    {
        NSString *path;
        path = [panel filename];
        [self runScript: path];       
    }
}

- (IBAction) rerunScript: (id) sender
{
    if (lastScript) [appCore runScript: lastScript];       
}

- (IBAction) back:(id)sender
{
    [appCore back];
}

- (IBAction) forward:(id)sender
{
    [appCore forward];
}

- (IBAction) selectSatellite:(id)sender
{
    if (sender &&
        [sender respondsToSelector: @selector(representedObject)] &&
        [sender representedObject])
    {
        [[appCore simulation] setSelection: [[[CelestiaSelection alloc] initWithCelestiaBody: [sender representedObject]] autorelease]];
    }
}

- (IBAction) showGLInfo:(id)sender
{
    if (![glInfoPanel isVisible])
    {
        NSTextStorage *text = [glInfo textStorage];
        NSAttributedString *str = [[NSAttributedString alloc] initWithString: [CGLInfo info]];
        [text setAttributedString: str];
        [str release];
    }

    [glInfoPanel makeKeyAndOrderFront: self];
}

- (IBAction) showInfoURL:(id)sender
{
    [appCore showInfoURL];
}

- (void) moviePanelDidEnd:(NSSavePanel*)savePanel returnCode: (int) rc contextInfo: (void *) ci
{
//    if (rc == NSOKButton )
    if (rc == 0 ) return;
    {
        NSString *path;
        path = [savePanel filename];
        NSLog(@"Saving movie: %@",path);       
		[appCore captureMovie: path width: 640 height: 480 frameRate: 30 ];
    }
}

- (IBAction) captureMovie: (id) sender
{
//  Remove following line to enable movie capture...
	NSRunAlertPanel(NSLocalizedString(@"No Movie Capture",@""), NSLocalizedString(@"Movie capture is not available in this version of Celestia.",@""),nil,nil,nil); return;

    NSSavePanel* panel = [NSSavePanel savePanel];
	NSString* lastMovie = nil; // temporary; should be saved in defaults

	[panel setRequiredFileType: @"mov"];
	[panel setTitle: NSLocalizedString(@"Capture Movie",@"")];
    [ panel beginSheetForDirectory:  [lastMovie stringByDeletingLastPathComponent]
                              file: [lastMovie lastPathComponent]
                    modalForWindow: [glView window]
                     modalDelegate: self
                    didEndSelector: @selector(moviePanelDidEnd:returnCode:contextInfo:)
                       contextInfo: nil
    ];
}

// GUI Tag Methods ----------------------------------------------------------

- (BOOL) validateMenuItem: (id) item
{
    if ( [startupCondition condition] == 0 ) return NO;
    if ( [item action] == nil  ) return NO;
    if ( [item action] != @selector(activateMenuItem:) ) return YES;
    if ( [item tag] == 0 )
    {
        return [item isEnabled];
    }
    else
    {
        return [settings validateItem: item ];
    }
}

- (IBAction) activateMenuItem: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        if ( tag < 0 ) // simulate key press and hold
        {
            [self keyPress: -tag hold: 2];
        } 
        else 
        {
            [settings actionForItem: item ];
        }
        [settings validateItemForTag: tag];
    }
}

-(IBAction) showPanel: (id) sender
{
    switch( [sender tag] )
    {
        case 0:
            if (!browserWindowController) browserWindowController = [[BrowserWindowController alloc] init];
            [browserWindowController showWindow: self];
            break;
        case 1:
            if (!eclipseFinderController) eclipseFinderController = [[EclipseFinderController alloc] init];
            [eclipseFinderController showWindow: self];
            break;
    }
}

- (void) showHelp: (id) sender
{
    if (helpWindowController == nil)
        helpWindowController = [[NSWindowController alloc] initWithWindowNibName: @"HelpWindow"];

    [helpWindowController showWindow: self];
}

@end

#pragma mark -

// Solution for keyDown sent but keyUp not sent for Cmd key combos.
// Fixes the infamous Cmd+arrow "infinite spin"
@interface CelestiaApplication : NSApplication
@end

@implementation CelestiaApplication
- (void)sendEvent: (NSEvent *)aEvent
{
    if ([aEvent type] == NSKeyUp)
    {
        [[[self mainWindow] firstResponder] tryToPerform: @selector(keyUp:)
                                                    with: aEvent];
        return;
    }
    
    [super sendEvent: aEvent];
}
@end
