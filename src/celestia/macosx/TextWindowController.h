//
//  TextWindowController.h
//  celestia
//
//  Created by Da Woon Jung on 2006-10-01.
//  Copyright 2006 Da Woon Jung. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface TextWindowController : NSWindowController
{
    IBOutlet NSTextField *textView;
}
- (void)makeActiveWithDelegate: (id)aDelegate;
@end
