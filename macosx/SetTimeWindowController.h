//
//  SetTimeWindowController.h
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

@interface SetTimeWindowController : NSWindowController
{
    IBOutlet NSTextField *dateField;
    IBOutlet NSTextField *timeField;
}
- (IBAction)setTime:(id)sender;
- (IBAction)showWindow:(id)sender;
@end
