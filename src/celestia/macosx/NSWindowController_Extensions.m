//
//  NSWindowController_Extensions.m
//  celestia
//
//  Created by Da Woon Jung on 10/1/08.
//  Copyright 2008 Celestia Development Team
//

#import "NSWindowController_Extensions.h"


@implementation NSWindowController (CelestiaWindowController)

- (void)keyDown:(NSEvent *)theEvent
{
    [[NSApp delegate] tryToPerform:@selector(delegateKeyDown:) with:theEvent];
}
@end
