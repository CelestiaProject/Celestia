//
//  FullScreenWindow.m
//  celestia
//
//  Created by Da Woon Jung on Wed Jul 21 2004.
//  Copyright 2005 Da Woon Jung. All rights reserved.
//

#import "FullScreenWindow.h"


@implementation FullScreenWindow

- (id) initWithScreen: (NSScreen *) screen
{
    CGDirectDisplayID displayID = screen ? (CGDirectDisplayID)[[[screen deviceDescription] objectForKey:@"NSScreenNumber"] intValue] : kCGDirectMainDisplay;

    NSRect fullScreenRect = NSMakeRect(0, 0, CGDisplayPixelsWide(displayID), CGDisplayPixelsHigh(displayID));

    self = [super initWithContentRect:fullScreenRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES screen:screen];

    return self;
}

- (BOOL) canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (void) fadeOutScreen
{
    CGDirectDisplayID displayID = (CGDirectDisplayID)[[[[self screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue];
    
    // Fade to black
    CGGammaValue redMin, redMax, redGamma, greenMin, greenMax, greenGamma, blueMin, blueMax, blueGamma;
    double fadeValue;
    
    CGGetDisplayTransferByFormula(displayID,
                                  &redMin, &redMax, &redGamma,
                                  &greenMin, &greenMax, &greenGamma,
                                  &blueMin, &blueMax, &blueGamma);
    
    for (fadeValue = 1.0; fadeValue >= 0.0; fadeValue -= 0.2 ) {
        CGSetDisplayTransferByFormula(displayID,
                                      redMin, fadeValue*redMax, redGamma,
                                      greenMin, fadeValue*greenMax, greenGamma,
                                      blueMin, fadeValue*blueMax, blueGamma);
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow: 0.05]];
    }    
}

- (void) restoreScreen
{
    CGDisplayRestoreColorSyncSettings();
}
@end
