//
//  BrowserWindowController.h
//  celestia
//
//  Created by Hank Ramsey on Sun Dec 25 2004.
//  Copyright (c) 2002 Chris Laurel et al. All rights reserved.
//
#define __AIFF__
#import <Cocoa/Cocoa.h>

@interface BrowserWindowController : NSWindowController
{
    NSBrowser* browser;
}

- (IBAction) go: (id) sender;

@end