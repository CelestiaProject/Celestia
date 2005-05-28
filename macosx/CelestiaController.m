//
//  CelestiaController.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#include <unistd.h>
#import <Cocoa/Cocoa.h>
#import "CelestiaController.h"
#import "FavoritesDrawerController.h"
#import "CelestiaOpenGLView.h"
#import "FullScreenWindow.h"
#import <Carbon/Carbon.h>
#import "CGLInfo.h"

#include <float.h>

#define CELESTIA_RESOURCES_FOLDER @"CelestiaResources"


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
    ready = NO;
    isDirty = YES;
    isFullScreen = NO;
    appCore = nil;
    fatalErrorMessage = nil;
    lastScript = nil;
	
    [self setupResourceDirectory];

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
    
    startupCondition = [[NSConditionLock alloc] initWithCondition: 0];

    // start initialization thread
#ifdef USE_THREADEDLOAD
    [NSThread detachNewThreadSelector: @selector(startInitialization) toTarget: self
    withObject: nil];

    // wait for completion
    [self performSelectorOnMainThread: @selector(waitWhileLoading:) withObject: nil waitUntilDone: NO ];
#else
    [self performSelector:@selector(startInitialization) withObject:nil afterDelay:0];
#endif
}


- (void) setupResourceDirectory
{
    NSBundle* mainBundle = [NSBundle mainBundle];
    // Change directory to resource dir so Celestia can find cfg files and textures
    NSFileManager *fileManager = [NSFileManager defaultManager]; 
    NSString* path;
    BOOL isFolder = NO;

    if ( [ fileManager fileExistsAtPath: path = [[[ mainBundle bundlePath ]  stringByDeletingLastPathComponent] stringByAppendingPathComponent: CELESTIA_RESOURCES_FOLDER ] isDirectory: &isFolder ] && isFolder )
        [fileManager changeCurrentDirectoryPath: path];
    else
    {
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
                    if ([fileManager fileExistsAtPath: path = [path stringByAppendingPathComponent: CELESTIA_RESOURCES_FOLDER] isDirectory: &isFolder] && isFolder)
                    {
                        break;
                    }
                }

                path = nil;
            }
        }

        if (path == nil)
        {
            if (![fileManager fileExistsAtPath: path = [[ mainBundle resourcePath ] stringByAppendingPathComponent: CELESTIA_RESOURCES_FOLDER ] isDirectory: &isFolder] || !isFolder)
            {
                NSRunAlertPanel(@"Missing Resource Directory",@"It appears that the \"CelestiaResources\" directory has not been properly installed in the correct location as indicated in the installation instructions. \n\nPlease correct this and try again.",nil,nil,nil);
                path = [mainBundle resourcePath];
            }
        }

        if (path)
            [fileManager changeCurrentDirectoryPath: path];
    }
}

- (void)startInitialization
{
#ifdef DEBUG
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif
#ifndef USE_THREADEDLOAD
    [loadingPanel makeKeyAndOrderFront: nil];
    [loadingPanel displayIfNeeded];
    [loadingIndicator setUsesThreadedAnimation: YES];
    [loadingIndicator startAnimation: nil];
#endif
    // initialize simulator in separate thread to allow loading indicator window while waiting
#ifdef DEBUG
    NSDate *t = [NSDate date];
#endif
    if (![appCore initSimulation])
    {
        [startupCondition lock];
        [startupCondition unlockWithCondition: 99];
#ifndef USE_THREADEDLOAD
        [loadingPanel orderOut: nil];
#endif
#ifdef DEBUG
        [pool release];
#endif
        [self fatalError: @"Error loading data files. Celestia will now quit."];
        [self fatalError: nil];
        [NSThread exit];
        return;
    }

#ifdef DEBUG
    NSLog(@"Init took %lf seconds\n", -[t timeIntervalSinceNow]);
#endif
    [startupCondition lock];
    [startupCondition unlockWithCondition: 1];
#ifndef USE_THREADEDLOAD
    [loadingPanel orderOut: nil];
    // complete startup
    [self fatalError: nil];
    [self finishInitialization];
#endif

#ifdef DEBUG
    [pool release];
#endif
}


- (void) fatalError: (NSString *) msg
{
    // handle fatal error message from either main or loading threads
    if ( msg == nil )
    {
        if (fatalErrorMessage == nil) return;
        [loadingPanel orderOut: nil ];
        NSRunAlertPanel(@"Fatal Error", fatalErrorMessage, nil, nil, nil);
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
        [loadingIndicator startAnimation: nil];
        // begin modal session for loading panel
        session = [NSApp beginModalSessionForWindow:loadingPanel];
        // modal session runloop
        for (;;) 
        {
            if ( fatalErrorMessage != nil )
                break;
            if ([NSApp runModalSession:session] != NSRunContinuesResponse)
                break;
            if ( [startupCondition condition] != 0 )
                break;
        }
        // terminate modal session for loading panel
        [NSApp endModalSession:session];
    }
    // check for fatal error in loading thread
    [self fatalError: nil];
    // complete startup
    [loadingPanel orderOut: nil ];
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
/*
-(void) setupRenderPanel
{
//    [[renderPanelController window] setAlphaValue: 0.8f];
//    [[renderPanelController window] setLevel: NSFloatingWindowLevel];
//    [[glView window] addChildWindow: [renderPanelController window] ordered: NSWindowAbove];
    [[renderPanelController window] setMovableByWindowBackground: YES];
    [[renderPanelController window] setHidesOnDeactivate: YES];
    [[renderPanelController window] setReleasedWhenClosed: NO];
    [[renderPanelController window] setOneShot: NO];
//    [renderPanelController finishSetup];
}
*/
-(void) startGLView
{
    [[glView window] setAutodisplay:YES];
    [[glView window] setHidesOnDeactivate: NO];
    [[glView window] setFrameUsingName: @"Celestia"];
    [[glView window] setAlphaValue: 1.0f];
    [[glView window] setFrameAutosaveName: @"Celestia"];
    [[glView window] makeMainWindow ];
    [[glView window] makeFirstResponder: glView ];
    [[glView window] makeKeyAndOrderFront: glView ];
    [glView registerForDraggedTypes:
        [NSArray arrayWithObjects: NSStringPboardType, NSFilenamesPboardType, NSURLPboardType, nil]];
    [glView setNeedsDisplay:YES];
}

- (void)finishInitialization
{
    [appCore initRenderer];

    [self setupFavorites];

    settings = [CelestiaSettings shared];
    [settings setControl: self];
    [settings scanForKeys: [renderPanelController window]];
//    [self setupRenderPanel];
    [settings validateItems];

    // load settings
    [settings loadUserDefaults];
    // Have to delay going full screen until after view is started
    BOOL shouldGoFullScreen = isFullScreen;
    isFullScreen = NO;

    // paste URL if pending
    if (pendingUrl != nil )
    {
        [ appCore setStartURL: pendingUrl ];
    }

    // set the simulation starting time to the current system time
    [appCore start:[NSDate date] withTimeZone:[NSTimeZone defaultTimeZone]];

    ready = YES;
    timer = [[NSTimer timerWithTimeInterval: 0.01 target: self selector:@selector(timeDisplay) userInfo:nil repeats:YES] retain];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];

    [self startGLView];

    if (shouldGoFullScreen)
        [self toggleFullScreen: self];

    // run script if pending
    if (pendingScript != nil )
    {
        [self runScript: pendingScript ];
    }

    // workaround for fov problem
    if (pendingUrl) [appCore goToUrl: pendingUrl];
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

- (void) applicationDidBecomeActive:(NSNotification *) aNotification
{
    if (isFullScreen && aNotification && ([aNotification object] == NSApp))
    {
        [self unpauseFullScreen];
    }
}

- (void) applicationWillHide:(NSNotification *) aNotification
{
    if (isFullScreen && aNotification && ([aNotification object] == NSApp))
    {
        [self pauseFullScreen];
    }
}

- (void) applicationWillResignActive:(NSNotification *) aNotification
{
    if (isFullScreen && aNotification && ([aNotification object] == NSApp) && ![[aNotification object] isHidden])
    {
        // Hiding also causes deactivation - handle hiding separately
        [self pauseFullScreen];
        [[self window] orderBack: self];
    }
}

/* On a multi-screen setup, user is able to change the resolution of the screen running Celestia from a different screen, or the menu bar position so handle that */
- (void)applicationDidChangeScreenParameters:(NSNotification *) aNotification
{
    if (isFullScreen && aNotification && ([aNotification object] == NSApp))
    {
        // If menu bar not on same screen, don't hide it anymore
        if (![self hideMenuBarOnActiveScreen])
            ShowMenuBar();

        NSScreen *screen = [[self window] screen];
        NSRect screenRect = [screen frame];

        if (!NSEqualSizes(screenRect.size, [[self window] frame].size))
            [[self window] setFrame: screenRect display:YES];
    }
}

-(BOOL)applicationShouldTerminate:(id)sender
{
    if (isFullScreen)
        [self pauseFullScreen];   // allow dialog to show

   if (  NSRunAlertPanel(@"Quit Celestia?",@"Are you sure you want to quit Celestia?",@"Quit",@"Cancel",nil) != NSAlertDefaultReturn ) 
   {
        if (isFullScreen)
            [self unpauseFullScreen];
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

    [browserWindowController release];
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
        NSArray *mainSubViews = [[mainWindow contentView] subviews];

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
        else if ([[mainWindow contentView] isKindOfClass: viewClass])
            windowedView = [mainWindow contentView];

        [mainWindow makeKeyAndOrderFront: self];

        if (windowedView == nil)
        {
            // Can't switch back to windowed mode, but hide full screen window
            // so user can still quit the program
            [[self window] orderOut: self];
            ShowMenuBar();
            [self fatalError: @"Unable to properly exit full screen mode. Celestia will now quit."];
            [self performSelector:@selector(fatalError:) withObject:nil afterDelay:0.1];
            return;
        }

        [windowedView setOpenGLContext: [glView openGLContext]];
        [[glView openGLContext] setView: windowedView];

        [[self window] close];  // full screen window releases on close
        ShowMenuBar();
        [self setWindow: mainWindow];
        glView = windowedView;
        [self setDirty];
        isFullScreen = NO;
        return;
    }

    // We will take over the screen that the window is on
    // (if there are >1 screens, the 50% rule applies)
    NSScreen *screen = [[glView window] screen];

    CelestiaOpenGLView *fullScreenView = [[CelestiaOpenGLView alloc] initWithFrame:[glView frame] pixelFormat:[glView pixelFormat]];
    [fullScreenView setMenu: [glView menu]];    // context menu

    FullScreenWindow *fullScreenWindow = [[FullScreenWindow alloc] initWithScreen: screen];

    [fullScreenWindow setBackgroundColor: [NSColor blackColor]];
    [fullScreenWindow setReleasedWhenClosed: YES];
    [fullScreenWindow setLevel: NSStatusWindowLevel];
    [self setWindow: fullScreenWindow];
    [fullScreenWindow setDelegate: self];
    // Hide the menu bar only if it's on the same screen
    [self hideMenuBarOnActiveScreen];
    [fullScreenWindow makeKeyAndOrderFront: nil];

    [fullScreenWindow setContentView: fullScreenView];
    [fullScreenView release];
    [fullScreenView setOpenGLContext: [glView openGLContext]];
    [[glView openGLContext] setView: fullScreenView];

    // Remember the original (bordered) window
    mainWindow = [glView window];
    // Close the original window (does not release it)
    [mainWindow close];
    glView = fullScreenView;
    [glView takeValue: self forKey: @"controller"];
    [fullScreenWindow makeFirstResponder: glView];
    // Make sure the view looks ready before unfading from black
    [glView update];
    [glView display];

    CGDisplayRestoreColorSyncSettings();
    isFullScreen = YES;
}

/* Lowers the level of a full-screen window (to allow Cmd-Tabbing, etc) */
- (void) pauseFullScreen
{
    [[self window] setLevel: NSNormalWindowLevel];
    ShowMenuBar();
}

/* Resumes full screen after a pauseFullScreen */
- (void) unpauseFullScreen
{
    [self hideMenuBarOnActiveScreen];
    [[self window] setLevel: NSStatusWindowLevel];
}

- (BOOL) hideMenuBarOnActiveScreen
{
    NSScreen *screen = [[self window] screen];
    NSArray *allScreens = [NSScreen screens];
    if (allScreens && [allScreens objectAtIndex: 0]!=screen)
        return NO;

    HideMenuBar();
    return YES;
}

- (void) runScript: (NSString*) path
{
        NSString* oldScript = lastScript;
        lastScript = [path retain];
        [oldScript autorelease];
        [appCore runScript: lastScript];       
}

- (void) openPanelDidEnd:(NSOpenPanel*)openPanel returnCode: (int) rc contextInfo: (void *) ci
{
    if (rc == NSOKButton )
    {
        NSString *path;
        path = [openPanel filename];
        [self runScript: path];       
    }
}

- (IBAction) openScript: (id) sender
{
    NSOpenPanel* panel = [[NSOpenPanel alloc] init]; //[NSOpenPanel openPanel];
    if (!lastScript) lastScript = [[NSHomeDirectory() stringByAppendingString: @"/"]  retain];

    [ panel beginSheetForDirectory:  [lastScript stringByDeletingLastPathComponent]
        file: [lastScript lastPathComponent]
        types: [NSArray arrayWithObjects: @"cel", @"celx", nil ]
        modalForWindow: [glView window]
        modalDelegate: self
        didEndSelector: @selector(openPanelDidEnd:returnCode:contextInfo:)
        contextInfo: nil
    ];
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

-(IBAction)showGLInfo:(id)sender
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

// GUI Tag Methods ----------------------------------------------------------

- (BOOL)     validateMenuItem: (id) item
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

- (void) addSurfaceMenu: (NSMenu*) contextMenu
{
    [settings addSurfaceMenu: contextMenu ];
}


-(IBAction) showPanel: (id) sender
{
    switch( [sender tag] )
    {
        case 0:
            if (!browserWindowController) browserWindowController = [[BrowserWindowController alloc] init];
            [browserWindowController showWindow: self];
            break;
    }
}

- (void) showHelp: (id) sender
{
    NSString *path = [[NSBundle mainBundle] pathForResource:@"KbdMouseJoyControls"
                                                     ofType:@"txt"];
    
    if (path)
        [[NSWorkspace sharedWorkspace] openFile:path];
}

@end
