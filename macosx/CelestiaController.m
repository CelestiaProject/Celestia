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

@implementation CelestiaController

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
    if (ready)
    {
        if (isDirty) [self resize];
        [appCore draw];
    } else {
        [self finishInitialization];
    }
}

- (void)idle
{
    //NSLog(@"[CelestiaController idle]");
    if (ready) {
        [appCore tick];
        [glView setNeedsDisplay:YES];
    }
}

- (void)awakeFromNib
{
    if ([[self superclass] instancesRespondToSelector:@selector(awakeFromNib)]) {
        [super awakeFromNib];
    }
    NSLog(@"[CelestiaController awakeFromNib]");
    [self startInitialization];
}
- (void)startInitialization
{
    NSLog(@"[CelestiaController startInitialization]");

    ready = NO;
    isDirty = YES;
    appCore = nil;
    timer = nil;
    // Change directory to resource dir so Celestia can find cfg files and textures
    chdir([[[NSBundle mainBundle] resourcePath] cString]);
    appCore = [CelestiaAppCore sharedAppCore];
    if (appCore == nil)
    {
        NSLog(@"Could not create CelestiaAppCore!");
        [NSApp terminate:self];
        return;
    }
    if (![ appCore initSimulation])
    {
        NSLog(@"Could not initSimulation!");
        [NSApp terminate:self];
        return;
    }
    timer = [[NSTimer scheduledTimerWithTimeInterval: 0.001 target: self selector:@selector(idle) userInfo:nil repeats:true] retain];
    [[glView window] setAutodisplay:YES];
    [glView setNeedsDisplay:YES];
}

- (void)finishInitialization
{
    NSInvocation *menuCallback;
    NSLog(@"finishInitialization");
    // GL should be all set up, now initialize the renderer.
    [appCore initRenderer];
    // Set the simulation starting time to the current system time
    [appCore start:[NSDate date] withTimeZone:[NSTimeZone defaultTimeZone]];
    ready = YES;
    menuCallback = [NSInvocation invocationWithMethodSignature:[FavoritesDrawerController instanceMethodSignatureForSelector:@selector(synchronizeFavoritesMenu)]];
    [menuCallback setSelector:@selector(synchronizeFavoritesMenu)];
    [menuCallback setTarget:favoritesDrawerController];
    [[CelestiaFavorites sharedFavorites] setSynchronize:menuCallback];
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

@end