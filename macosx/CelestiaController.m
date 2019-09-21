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
#import "ConfigSelectionWindowController.h"

#include <float.h>

@implementation CelestiaController

static NSURL *configFilePath = nil;
static NSURL *dataDirPath = nil;
static NSURL *extraDataDirPath = nil;

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

    // read config file/data dir from saved
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSData *configFileData = [prefs objectForKey:configFilePathPrefKey];
    NSData *dataDirData = [prefs objectForKey:dataDirPathPrefKey];

    // access saved bookmarks
    if (configFileData != nil)
    {
        NSError *error = nil;
        NSURL *tempPath = [NSURL URLByResolvingBookmarkData:configFileData options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:nil error:&error];
        if ([tempPath startAccessingSecurityScopedResource])
        {
            configFilePath = [tempPath retain];
        }
    }

    if (dataDirData != nil)
    {
        NSError *error = nil;
        NSURL *tempPath = [NSURL URLByResolvingBookmarkData:dataDirData options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:nil error:&error];
        if ([tempPath startAccessingSecurityScopedResource])
        {
            dataDirPath = [tempPath retain];
        }
    }

    // use the default location for nil ones
    if (configFilePath == nil)
        configFilePath = [[ConfigSelectionWindowController applicationConfig] retain];

    if (dataDirPath == nil)
        dataDirPath = [[ConfigSelectionWindowController applicationDataDirectory] retain];

    // add the edit configuration menu item
    NSMenu *appMenu = [[[[NSApp mainMenu] itemArray] objectAtIndex:0] submenu];
    [appMenu insertItem:[[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Change Configuration File", "") action:@selector(changeConfigFileLocationFromMenu) keyEquivalent:@""] atIndex:[[appMenu itemArray] count] - 1];

    ready = NO;
    isDirty = YES;
    isFullScreen = NO;
    needsRelaunch = NO;
    forceQuit = NO;
    appCore = nil;
    fatalErrorMessage = nil;
    lastScript = nil;

    [self setupResourceDirectory];
    [scriptsController buildScriptMenuWithScriptDir:[extraDataDirPath path]];

    //  hide main window until ready
    [[glView window] setAlphaValue: 0.0f];  //  not  [[glView window] orderOut: nil];
    [[glView window] setIgnoresMouseEvents:YES];

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

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        // start initialization thread
        BOOL result = [self startInitialization];
        dispatch_async(dispatch_get_main_queue(), ^{
            [splashWindowController close];
            if (result)
                [self finishInitialization];
            else
                [self initializationError];
        });
    });
}

- (void)setNeedsRelaunch:(BOOL)newValue
{
    needsRelaunch = newValue;
}

- (void)changeConfigFileLocationFromMenu
{
    [self changeConfigFileLocation:YES];
}

- (void)changeConfigFileLocation:(BOOL)cancelAllowed
{
    if (configSelectionWindowController == nil) {
        configSelectionWindowController = [[ConfigSelectionWindowController alloc] initWithWindowNibName:@"ConfigSelectionWindow"];
        configSelectionWindowController->dataDirPath = [dataDirPath retain];
        configSelectionWindowController->configFilePath = [configFilePath retain];
    }
    [configSelectionWindowController setMandatory:!cancelAllowed];
    [configSelectionWindowController showWindow:self];
}

- (void) setupResourceDirectory
{
    // Change directory to resource dir so Celestia can find cfg files and textures
    NSFileManager *fileManager = [NSFileManager defaultManager];
    [fileManager changeCurrentDirectoryPath:[dataDirPath path]];

    // extra/script resources are located in application support folder, not sandboxed
    NSString *supportPath = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject];
    NSString *extraDataDir = [supportPath stringByAppendingPathComponent:CELESTIA_RESOURCES_FOLDER];
    NSString *extraDir = [extraDataDir stringByAppendingPathComponent:@"extras"];
    NSString *scriptDir = [extraDataDir stringByAppendingPathComponent:CEL_SCRIPTS_FOLDER];
    BOOL isDirectory;
    BOOL exists = [fileManager fileExistsAtPath:extraDataDir isDirectory:&isDirectory];

    if (exists && !isDirectory) // should be a directory but not
        return;

    if (!exists)
    {
        // create directory and subdirectories
        if ([fileManager createDirectoryAtPath:extraDataDir withIntermediateDirectories:YES attributes:nil error:nil])
        {
            [fileManager createDirectoryAtPath:extraDir withIntermediateDirectories:YES attributes:nil error:nil];
            [fileManager createDirectoryAtPath:scriptDir withIntermediateDirectories:YES attributes:nil error:nil];
        }
    }

    if ([fileManager fileExistsAtPath:extraDataDir isDirectory:&isDirectory] && isDirectory) {
        extraDataDirPath = [NSURL fileURLWithPath:extraDataDir];
    }
}

- (BOOL)startInitialization
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    [[glView openGLContext] makeCurrentContext];
#ifdef DEBUG
    NSDate *t = [NSDate date];
#endif

    BOOL result = [appCore initSimulationWithConfigPath:[configFilePath path] extraPath:[extraDataDirPath path]];

    if (!result)
    {
        [startupCondition lock];
        [startupCondition unlockWithCondition:99];
    }
    else
    {
#ifdef DEBUG
        NSLog(@"Init took %lf seconds\n", -[t timeIntervalSinceNow]);
#endif
        [startupCondition lock];
        [startupCondition unlockWithCondition:1];
    }

    [pool release];
    return result;
}

- (void) initializationError
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"Celestia failed to load data files.", @"")];
    [alert addButtonWithTitle:NSLocalizedString(@"Choose Configuration File", @"")];
    [alert addButtonWithTitle:NSLocalizedString(@"Quit", @"")];
    forceQuit = YES;
    if ([alert runModal] == NSAlertFirstButtonReturn)
    {
        // choose new configuration file
        [self changeConfigFileLocation:NO];
        return;
    }
    // quit
    [NSApp terminate:self];
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
    [[glView window] setIgnoresMouseEvents:NO];
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
    if (forceQuit || needsRelaunch)
        return YES;

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

    [configFilePath stopAccessingSecurityScopedResource];
    [dataDirPath stopAccessingSecurityScopedResource];
    [configFilePath release];
    [dataDirPath release];

    [lastScript release];
    [eclipseFinderController release];
    [browserWindowController release];
    [helpWindowController release];

    if (appCore != nil) {
        [appCore release];
        appCore = nil;
    }

    // TODO: relaunch app in sandbox
//    if (needsRelaunch)
//    {
//         [[NSWorkspace sharedWorkspace] launchAppWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier] options:NSWorkspaceLaunchAsync additionalEventParamDescriptor:nil launchIdentifier:nil];
//    }
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
    [glView setNeedsDisplay:YES];
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
    [glView setValue: self forKey: @"controller"];
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
