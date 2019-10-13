//
//  NSWindowController_Extensions.h
//  celestia
//
//  Created by Da Woon Jung on 10/1/08.
//  Copyright 2008 Celestia Development Team
//

#import <Cocoa/Cocoa.h>

@protocol CelestiaWindowKeyDownNotifier <NSObject>
- (void)delegateKeyDown:(NSEvent *)theEvent;
@end

@interface NSWindowController (CelestiaWindowController)
- (void)keyDown:(NSEvent *)theEvent;
@end
