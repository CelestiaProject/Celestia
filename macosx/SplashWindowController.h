//
//  SplashWindowController.h
//  celestia
//
//  Created by Da Woon Jung on 2005-12-31.
//  Copyright 2005 Chris Laurel. All rights reserved.
//


@interface SplashWindowController : NSObject
{
    IBOutlet NSTextField *status;
    IBOutlet NSTextField *version;
    IBOutlet NSWindow *window;
}
- (void)setStatusText: (NSString *)statusText;
- (void)showWindow;
- (void)close;
- (NSWindow *)window;
@end
