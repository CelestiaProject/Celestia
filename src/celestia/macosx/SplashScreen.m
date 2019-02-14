//
//  SplashScreen.m
//  celestia
//
//  Created by Da Woon Jung on 2005-12-18.
//  Copyright 2005 Chris Laurel. All rights reserved.
//

#import "SplashScreen.h"


@implementation SplashImageView

- (BOOL)mouseDownCanMoveWindow
{
    return YES;
}
@end


@implementation SplashWindow

- (id)initWithContentRect:(NSRect)contentRect 
                styleMask:(unsigned int)aStyle 
                  backing:(NSBackingStoreType)bufferingType 
                    defer:(BOOL)flag
{
    if (self = [super initWithContentRect: contentRect 
                                styleMask: NSBorderlessWindowMask
                                  backing: NSBackingStoreBuffered
                                    defer:YES])
    {
        [self setReleasedWhenClosed: YES];
        [self setBackgroundColor: [NSColor clearColor]];
        [self setAlphaValue: 1.0f];
        [self setOpaque: NO];
        [self setHasShadow: NO];    // image already has drop shadow
        if ([self respondsToSelector: @selector(setMovableByWindowBackground:)])
            [self setMovableByWindowBackground: YES];
        
//        [self center];    // does not center exactly so don't use
        NSScreen *screen = [self screen];
        NSRect screenFrame = [screen frame];
        [self setFrameOrigin:
            NSMakePoint((NSWidth(screenFrame) - NSWidth(contentRect))/2, 
                        (NSHeight(screenFrame) - NSHeight(contentRect))/2)];
    }

    return self;
}
@end
