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
    ready = NO;
    isDirty = YES;
    appCore = nil;
    fatalErrorMessage = nil;
	
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
    [NSThread detachNewThreadSelector: @selector(startInitialization) toTarget: self
    withObject: nil];

    // wait for completion
    [self performSelectorOnMainThread: @selector(waitWhileLoading:) withObject: nil waitUntilDone: NO ];
}


- (void) setupResourceDirectory
{
    NSBundle* mainBundle = [NSBundle mainBundle];
    // Change directory to resource dir so Celestia can find cfg files and textures
    NSFileManager *fileManager = [NSFileManager defaultManager]; 
    NSString* path; 
    if ( [ fileManager fileExistsAtPath: path = [[[ mainBundle bundlePath ]  stringByDeletingLastPathComponent] stringByAppendingPathComponent: @"CelestiaResources" ]] )
        chdir([path cString]);
    else if ( [ fileManager fileExistsAtPath: path = [ @"~/Library/Application Support/CelestiaResources" stringByExpandingTildeInPath] ] )
        chdir([path cString]);
    else if ( [ fileManager fileExistsAtPath: path = @"/Library/Application Support/CelestiaResources" ] )
        chdir([path cString]);
    else if ( [ fileManager fileExistsAtPath: path = [[ mainBundle resourcePath ] stringByAppendingPathComponent: @"CelestiaResources" ]] )
        chdir([path cString]);
    else {
        NSRunAlertPanel(@"Missing Resource Directory",@"It appears that the \"CelestiaResources\" directory has not been properly installed in the correct location as indicated in the installation instructions. \n\nPlease correct this and try again.",nil,nil,nil);
        chdir([[mainBundle resourcePath] cString]);
        }
}

- (void)startInitialization
{
    // initialize simulator in separate thread to allow loading indicator window while waiting
    if (![appCore initSimulation])
    {
        NSLog(@"Could not initSimulation!");
        [startupCondition lock];
        [startupCondition unlockWithCondition: 99];
        [NSThread exit];
        return;
    }

    [startupCondition lock];
    [startupCondition unlockWithCondition: 1];
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
    [self setupRenderPanel];
    [settings validateItems];

    // load settings
    [settings loadUserDefaults];

    // set the simulation starting time to the current system time
    [appCore start:[NSDate date] withTimeZone:[NSTimeZone defaultTimeZone]];

    // run script if pending
    if (pendingScript != nil )
    {
        [appCore runScript: pendingScript ];
    }

    // paste URL if pending
    if (pendingUrl != nil )
    {
        [ appCore goToUrl: pendingUrl ];
    }
        
    ready = YES;
    [self startGLView];

    // start animation timer
    timer = [[NSTimer scheduledTimerWithTimeInterval: 0.01 target: self selector:@selector(timeDisplay) userInfo:nil repeats:true] retain];

}

// Application Event Handler Methods ----------------------------------------------------------

- (void) applicationWillFinishLaunching:(NSNotification *) notification {
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self andSelector:@selector( handleURLEvent:withReplyEvent: ) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (BOOL) application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    if ( ready )
        [appCore runScript: filename ];
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

-(BOOL)applicationShouldTerminate:(id)sender
{
   if (  NSRunAlertPanel(@"Quit Celestia?",@"Are you sure you want to quit Celestia?",@"Quit",@"Cancel",nil) != NSAlertDefaultReturn ) 
   {
   return NO;
   }
    if (timer != nil) {
        [timer invalidate];
        [timer release];
    }
    timer = nil;
    [[CelestiaAppCore sharedAppCore] archive];
    return YES;
}

- (void) applicationWillTerminate:(NSNotification *) notification {
    [settings storeUserDefaults];
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
    if (!ready) return;

    // check for time to release simulated key held down
    [self keyTick];

    // adjust timer if necessary to avoid saturating the runloop
    // otherwise appkit events may be delayed
    static NSEvent* lastEvent = nil;
     NSEvent* nextEvent = nil;

    nextEvent = nil;
    // make sure we get some events (so we don't block checking the event queue)
    [NSEvent startPeriodicEventsAfterDelay: 0.0 withPeriod: 0.001 ]; 
    // check for waiting events
    nextEvent = [NSApp nextEventMatchingMask: ( NSPeriodicMask|NSAppKitDefined ) untilDate: nil inMode: NSDefaultRunLoopMode dequeue: NO];
    // stop generating periodic events
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
    // force display update
    [self forceDisplay];
 }

// Application Action Methods ----------------------------------------------------------

- (void) openPanelDidEnd:(NSOpenPanel*)openPanel returnCode: (int) rc contextInfo: (void *) ci
{
    if (rc == NSOKButton )
    {
        NSString *path;
        path = [openPanel filename];
        [appCore runScript: path];        
    }
}

- (IBAction) openScript: (id) sender
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [ panel beginSheetForDirectory:  nil
        file: nil
        types: [NSArray arrayWithObjects: @"cel", @"celx", nil ]
        modalForWindow: [glView window]
        modalDelegate: self
        didEndSelector: @selector(openPanelDidEnd:returnCode:contextInfo:)
        contextInfo: nil
    ];
}

- (IBAction) back:(id)sender
{
    [appCore back];
}

- (IBAction) forward:(id)sender
{
    [appCore forward];
}

- (IBAction) showInfoURL:(id)sender
{
    [appCore showInfoURL];
}

// GUI Tag Methods ----------------------------------------------------------

- (BOOL)     validateMenuItem: (id) item
{
    if ( [startupCondition condition] == 0 ) return NO;
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

// Dealloc Method ----------------------------------------------------------

- (void)dealloc
{
    if (appCore != nil) {
        [appCore release];
    }
    appCore = nil;
    if (timer != nil) {
        [timer invalidate];
        [timer release];
    }
    timer = nil;
    [super dealloc];
}    
        

@end