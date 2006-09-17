//
//  FullScreenWindow.h
//  celestia
//
//  Created by Da Woon Jung on Wed Jul 21 2004.
//  Copyright 2005 Da Woon Jung. All rights reserved.
//


@interface FullScreenWindow : NSWindow

- (id) initWithScreen: (NSScreen *) screen;
- (void) fadeOutScreen;
- (void) restoreScreen;

@end
