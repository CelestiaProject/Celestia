//
//  BrowserWindowController.h
//  celestia
//
//  Created by Hank Ramsey on Sun Dec 25 2004.
//  Copyright (c) 2002 Chris Laurel et al. All rights reserved.
//

#import "NSWindowController_Extensions.h"

@interface BrowserWindowController : NSWindowController
{
    IBOutlet NSTabView *tabView;
    NSBrowser* browser;
    NSString * rootId;
}

- (IBAction) go: (id) sender;

@end
