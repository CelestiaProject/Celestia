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
    IBOutlet NSTextField *jdField;
    IBOutlet NSTextField *timeField;
    NSDateFormatter *dateTimeFormat;
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    NSDateFormatter *bcFormat;
    NSDateFormatter *zeroFormat;
#endif
    BOOL setupDone;
}
- (IBAction)setTime:(id)sender;
@end
