//
//  TextWindowController.h
//  celestia
//
//  Created by Da Woon Jung on 2006-10-01.
//  Copyright 2006 Da Woon Jung. All rights reserved.
//

#import "TextWindowController.h"

@implementation TextWindowController
- (id)init
{
    self = [super initWithWindowNibName: @"TextWindow"];
    return self;
}

- (void)makeActiveWithDelegate: (id)aDelegate
{
    [[self window] makeKeyAndOrderFront: nil];
    [[self window] setAlphaValue: 0.0f];
    [[self window] makeFirstResponder: textView];
    [textView setDelegate: aDelegate];
}
@end
