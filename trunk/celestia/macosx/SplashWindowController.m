//
//  SplashWindowController.m
//  celestia
//
//  Created by Da Woon Jung on 2005-12-31.
//  Copyright 2005 Chris Laurel. All rights reserved.
//

#import "SplashWindowController.h"


@implementation SplashWindowController

- (void)awakeFromNib
{
    if (version)
    {
        NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
        NSString *shortVersion = [infoDict objectForKey: @"CFBundleShortVersionString"];
#if DEBUG
        if (shortVersion == nil) shortVersion = @"";
        shortVersion = [shortVersion stringByAppendingString: @" DEBUG BUILD"];
#endif
        if (shortVersion && [shortVersion length]>0)
            [version setStringValue: shortVersion];
    }
}

- (void)setStatusText: (NSString *)statusText
{
    [status setStringValue: statusText];
    [status displayIfNeeded];
}

- (void)showWindow
{
    if (window)
    {
        [window makeKeyAndOrderFront: self];
        [window displayIfNeeded];
    }
}

- (void)close
{
    [window close];
    window = nil;   // released on close is set in SplashWindow
}

- (NSWindow *)window
{
    return window;
}
@end
