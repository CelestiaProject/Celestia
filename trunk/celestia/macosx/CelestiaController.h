//
//  CelestiaController.h
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <AppKit/AppKit.h>
#import "CelestiaAppCore.h"
#import "FavoritesDrawerController.h"

@interface CelestiaController : NSWindowController 
{
    CelestiaAppCore* appCore;
    BOOL ready;
    BOOL isDirty;
    IBOutlet NSOpenGLView *glView;
    IBOutlet NSWindow *mainWindow;
    IBOutlet NSWindow *gotoWindow;
    IBOutlet FavoritesDrawerController *favoritesDrawerController;
    NSTimer* timer;
}
-(BOOL)applicationShouldTerminate:(id)sender;
- (BOOL)windowShouldClose:(id)sender;
- (IBAction)showGotoObject:(id)sender;
- (IBAction)gotoObject:(id)sender;
- (void)setDirty;
- (void)resize;
- (void)startInitialization;
- (void)finishInitialization;
- (void)display;
- (void)idle;
- (void)awakeFromNib;

@end
