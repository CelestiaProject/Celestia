//
//  CelestiaController.h
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#import "CelestiaAppCore.h"
#import "CelestiaSettings.h"
#import "FavoritesDrawerController.h"
#import "RenderPanelController.h"
#import "BrowserWindowController.h"

#define CELESTIA_RESOURCES_FOLDER @"CelestiaResources"

@class CelestiaOpenGLView;
@class SplashWindow;
@class SplashWindowController;
@class EclipseFinderController;
@class ScriptsController;

@interface CelestiaController : NSWindowController 
{
    CelestiaSettings* settings;
    CelestiaAppCore* appCore;
    BOOL threaded;
    BOOL ready;
    BOOL isDirty;
    BOOL isFullScreen;
    IBOutlet SplashWindowController *splashWindowController;
    IBOutlet NSTextView *glInfo;
    IBOutlet NSPanel *glInfoPanel;
    IBOutlet CelestiaOpenGLView *glView;
    NSWindow *origWindow;
    IBOutlet FavoritesDrawerController *favoritesDrawerController;
    IBOutlet RenderPanelController *renderPanelController;
    IBOutlet ScriptsController *scriptsController;
    BrowserWindowController *browserWindowController;
    EclipseFinderController *eclipseFinderController;
    NSWindowController *helpWindowController;
    NSTimer* timer;

    NSConditionLock* startupCondition;
    int keyCode, keyTime;
    NSString* lastScript;
    NSString *pendingScript;
    NSString *pendingUrl;
}
-(BOOL)applicationShouldTerminate:(id)sender;
-(BOOL)windowShouldClose:(id)sender;
-(IBAction)back:(id)sender;
-(IBAction)forward:(id)sender;
-(IBAction)showGLInfo:(id)sender;
-(IBAction)showInfoURL:(id)sender;
-(void)runScript: (NSString*) path;
-(IBAction)openScript:(id)sender;
-(IBAction)rerunScript: (id) sender;
-(IBAction)toggleFullScreen:(id)sender;
-(BOOL)hideMenuBarOnActiveScreen;
-(void)setDirty;
-(void)forceDisplay;
-(void)resize;
-(void)startInitialization;
-(void)finishInitialization;
-(void)display;
-(void)awakeFromNib;
-(void)delegateKeyDown:(NSEvent *)theEvent;
-(void)keyPress:(int)code hold:(int)time;
-(void)setupResourceDirectory;
+(CelestiaController*) shared;
-(void) fatalError: (NSString *) msg;

-(IBAction) showPanel: (id) sender;

-(IBAction) captureMovie: (id) sender;

-(BOOL)validateMenuItem:(id)item;
-(IBAction)activateMenuItem:(id)item;

-(void)showHelp:(id)sender;
@end
