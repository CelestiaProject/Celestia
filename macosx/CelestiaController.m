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
    [[glView window] setFrameUsingName: @"Celestia"];
    [[glView window] setFrameAutosaveName: @"Celestia"];
    [[glView window] makeFirstResponder: glView ];
    [glView registerForDraggedTypes:
            [NSArray arrayWithObject: NSStringPboardType]];
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

- (IBAction)back:(id)sender
{
    [appCore back];
}

- (IBAction)forward:(id)sender
{
    [appCore forward];
}

- (BOOL) itemFlag: (id) item
{
    CelestiaRenderer *renderer;
    int tag = [item tag];
    BOOL label = NO;
    NSDictionary *flags;
    NSString *key = NULL;
    NSNumber* state;
    if (tag == 32 )
    {
       if(  fabs([[[ appCore simulation] getTimeScale] doubleValue]) == 0.0 ) 
            return YES;
        else
            return NO;
    }
    switch ( tag)
    {
        case 112: key = @"Planets"; label = YES; break;
        case 109: key = @"Moons"; label = YES; break;
        case 119: key = @"Asteroids"; label = YES; break;
        case  98: key = @"Stars"; label = YES; break;
        case 101: key = @"Galaxies"; label = YES; break;
        case 110: key = @"Spacecraft"; label = YES; break;
        case  61: key = @"Constellations"; label = YES; break;
        case 105: key = @"CloudMaps"; label = NO; break;
        case   1: key = @"Atmospheres"; label = NO; break;
        case  12: key = @"NightMaps"; label = NO; break;
        case   5: key = @"EclipseShadows"; label = NO; break;
        case 111: key = @"Orbits"; label = NO; break;
        case 117: key = @"Galaxies"; label = NO; break;
        case  47: key = @"Diagrams"; label = NO; break;
        case   2: key = @"Boundaries"; label = NO; break;
        case  59: key = @"CelestialSphere"; label = NO; break;
        case  25: key = @"AutoMag"; label = NO; break;
        case  20: key = @"CometTails"; label = NO; break;
        case  11: key = @"Markers"; label = NO; break;
        case  19: key = @"StarsAsPoints"; label = NO; break;
        case  24: key = @"SmoothLines"; label = NO; break;
        default : break;
    }
    if ( key == NULL ) return NO;
    renderer = [[CelestiaAppCore sharedAppCore] renderer];
    if ( label )
       flags = [renderer labelFlags];
    else
       flags = [renderer renderFlags];
    state = (NSNumber*) [ flags objectForKey: key ];
    if ( state == NULL ) return NO;
    return [state boolValue];    
}


- (BOOL)     validateMenuItem: (id) item
{
    if ( [item tag] == 0 )
    {
        return [item isEnabled];
    }
    else
    {
      if ( [self itemFlag: item] )
        [item setState: NSOnState ];
      else
        [item setState: NSOffState ];      
    }
    return YES;
}

- (IBAction) activateMenuItem: (id) item
{
    if ( [item tag] != 0 )
    {
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    if ( [item tag] < 0 )
        [self keyPress: -[item tag] hold: 2];
    else if ( [item tag] < 128 )
       [ appCore charEntered: [item tag] ];
    
    }
}


@end