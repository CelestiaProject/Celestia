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

@implementation CelestiaController

-(BOOL)applicationShouldTerminate:(id)sender
{
    if (timer != nil) {
        [timer invalidate];
        [timer release];
    }
    timer = nil;
    [[CelestiaAppCore sharedAppCore] archive];
    return YES;
}

-(BOOL)windowShouldClose:(id)sender
{
    [NSApp terminate:nil];
    return YES;
}

- (void)resize 
{
    //NSLog(@"[CelestiaController resize]");
    [appCore resize:[glView frame]];
    isDirty = NO;
}

- (void)setDirty
{
    isDirty = YES;
}

- (void)display 
{
    //NSLog(@"[CelestiaController display]");
    if (!ready) 
       [self finishInitialization];
//    else
    {
        if (isDirty) [self resize];
            [appCore tick];
            [appCore draw];
    }
//else {
 //       [self finishInitialization];
//    }
}

-(void) keyPress:(int) code hold: (int) time
{
    keyCode = code;
    keyTime = time;
    [appCore keyDown: keyCode ];
}

- (void)idle
{
    //NSLog(@"[CelestiaController idle]");
    if (ready)
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
    
        [appCore tick];
        [glView setNeedsDisplay:YES];
    }
}


- (void)awakeFromNib
{
    if ([[self superclass] instancesRespondToSelector:@selector(awakeFromNib)]) {
        [super awakeFromNib];
    }
    //NSLog(@"[CelestiaController awakeFromNib]");
    [self startInitialization];


}

- (void) setupResourceDirectory
{
    // Change directory to resource dir so Celestia can find cfg files and textures
    NSFileManager *fileManager = [NSFileManager defaultManager]; 
    NSString* path = [ @"~/Library/Application Support/CelestiaResources" stringByExpandingTildeInPath];
    if ( [ fileManager fileExistsAtPath: path ] )
        chdir([path cString]);
    else
        chdir([[[NSBundle mainBundle] resourcePath] cString]);
}

- (void)startInitialization
{
    //NSLog(@"[CelestiaController startInitialization]");

    ready = NO;
    isDirty = YES;
    appCore = nil;
    timer = nil;
    [ self setupResourceDirectory ];
NSLog(@"about to createAppCore\n");
    appCore = [CelestiaAppCore sharedAppCore];
    if (appCore == nil)
    {
        NSLog(@"Could not create CelestiaAppCore!");
        [NSApp terminate:self];
        return;
    }
NSLog(@"about to initSimulation\n");
    if (![ appCore initSimulation])
    {
        NSLog(@"Could not initSimulation!");
        [NSApp terminate:self];
        return;
    }
NSLog(@"done with initSimulation\n");
    timer = [[NSTimer scheduledTimerWithTimeInterval: 0.001 target: self selector:@selector(idle) userInfo:nil repeats:true] retain];
    [[glView window] setAutodisplay:YES];
    [glView setNeedsDisplay:YES];
    [[glView window] setFrameUsingName: @"Celestia"];
    [[glView window] setFrameAutosaveName: @"Celestia"];
    [[glView window] makeFirstResponder: glView ];
    [glView registerForDraggedTypes:
            [NSArray arrayWithObject: NSStringPboardType]];
    [self scanForKeys: [renderPanelController window]];
}

- (void)finishInitialization
{
    NSInvocation *menuCallback;
    //NSLog(@"finishInitialization");
    // GL should be all set up, now initialize the renderer.
    [appCore initRenderer];
    // Set the simulation starting time to the current system time
    [appCore start:[NSDate date] withTimeZone:[NSTimeZone defaultTimeZone]];
    ready = YES;
    menuCallback = [NSInvocation invocationWithMethodSignature:[FavoritesDrawerController instanceMethodSignatureForSelector:@selector(synchronizeFavoritesMenu)]];
    [menuCallback setSelector:@selector(synchronizeFavoritesMenu)];
    [menuCallback setTarget:favoritesDrawerController];
    [[CelestiaFavorites sharedFavorites] setSynchronize:menuCallback];
    [[CelestiaFavorites sharedFavorites] synchronize];
    // DEBUG
    //NSLog(@"%@",[CGLInfo displayDescriptions]);

    [renderPanelController finishSetup];
    [appCore validateItems];
}

- (void)dealloc
{
    NSLog(@"[CelestiaController dealloc]");
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


- (IBAction)gotoObject:(id)sender
{
    NSLog(@"[CelestiaController gotoObject:%@]",sender);
}

- (IBAction)showGotoObject:(id)sender
{
    NSLog(@"[CelestiaController showGotoObject:%@]",sender);
    [gotoWindow makeKeyAndOrderFront:self];
}

- (void) openPanelDidEnd:(NSOpenPanel*)openPanel returnCode: (int) rc contextInfo: (void *) ci
{
    if (rc == NSOKButton )
    {
        NSString *path;
        path = [openPanel filename];
        [openPanel close];
        [appCore runScript: path];
        
//       NSRunAlertPanel(@"Not yet implemented",@"Stay tuned.",nil,nil,nil);
    }
}

- (IBAction) openScript: (id) sender
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [ panel beginSheetForDirectory: nil
        file: nil
            types: nil // @".cel"
            modalForWindow: [glView window]
            modalDelegate: self
            didEndSelector: @selector(openPanelDidEnd:returnCode:contextInfo:)
            contextInfo: nil
    ];
}


- (IBAction)back:(id)sender
{
    [appCore back];
}

- (IBAction)forward:(id)sender
{
    [appCore forward];
}

- (BOOL)     validateMenuItem: (id) item
{
    if ( [item tag] == 0 )
    {
        return [item isEnabled];
    }
    else
    {
        return [appCore validateItem: item ];
    }
}

- (IBAction) activateMenuItem: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
        if ( tag < 0 )
            [self keyPress: -tag hold: 2];
        else [ appCore actionForItem: item ];
        [appCore validateItemForTag: tag];
    }
}

- (IBAction) activateSwitchButton: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        [ appCore actionForItem: item ];
        [ appCore tick]; [glView setNeedsDisplay:YES];
    }
}

- (IBAction) activateMatrixButton: (id) item
{
    item = [item selectedCell];
    int tag = [item tag];
    if ( tag != 0 )
    {
        [ appCore actionForItem: item ];
    }
}

- (void) scanForKeys: (id) item
{
//    NSLog(@"scanning item %@\n", [item description]);

    if ( [item isKindOfClass: [NSTabViewItem class]] )
    {
//	 NSLog(@"scanning viewitem\n");
        item = [item view];
        [ self scanForKeys: item ];
        return;
    }


    if ([item isKindOfClass: [NSMenuItem class]] && [item tag] != 0)
        {
//	 NSLog(@"scanning menuItem\n");
            [appCore defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateMenuItem:)];
            return; 
        }
    else if ([item isKindOfClass: [NSSlider class]] && [item tag] != 0)
        {
//            NSLog(@"scanning cell\n");
            [appCore defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateSwitchButton:)];
            return; 
        }

    if ( [item isKindOfClass: [NSTabView class]] )
    {
//	 NSLog(@"scanning tabview\n");
        item = [ [item tabViewItems] objectEnumerator ];
    }
    else if ( [item isKindOfClass: [NSPopUpButton class]] )
    {
        [appCore defineKeyForItem: item];
//	 NSLog(@"scanning popupbutton\n");
        item = [ [item itemArray] objectEnumerator ];
    }
    else if ([item isKindOfClass: [NSControl class]] && [item tag] != 0)
        {
//	 NSLog(@"scanning control\n");
            [appCore defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateSwitchButton:)];
            return; 
        }
    else if ( [item isKindOfClass: [NSMatrix class]] )
    {
//	 NSLog(@"scanning matrix\n");
        item = [ [item cells] objectEnumerator ];
    }
    else if ( [item isKindOfClass: [NSView class]] )
    {
//	 NSLog(@"scanning view\n");
//        NSLog(@"subviews items = %@\n", [item subviews]);
        item = [ [item subviews] objectEnumerator ];
//        NSLog(@"view items = %@\n", item);
    };

    if ( [item isKindOfClass: [NSEnumerator class] ] )
    {
//	 NSLog(@"scanning array\n");
        id subitem;
        while(subitem = [item nextObject])
        {
//            NSLog(@"scanning array  item\n");
            [ self scanForKeys: subitem ];
        }
        return;
    }

    if ([item isKindOfClass: [NSCell class]] && [item tag] != 0)
        {
//	 NSLog(@"scanning cell\n");
            [appCore defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateMatrixButton:)];
            return; 
        }

    if ( [item isKindOfClass: [NSWindow class]] )
    {
//        NSLog(@"scanning window\n");
        item = [item contentView ];
        [ self scanForKeys: item ];
        return;
    }
    
        
}

@end