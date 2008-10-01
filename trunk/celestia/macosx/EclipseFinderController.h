//
//  EclipseFinderController.h
//  celestia
//
//  Created by Da Woon Jung on 2007-05-07.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "NSWindowController_Extensions.h"

@interface EclipseFinderController : NSWindowController
{
    IBOutlet NSTextField *eclipseFindEnd;
    IBOutlet NSTextField *eclipseFindStart;
    IBOutlet NSTableView *eclipseList;
    IBOutlet NSProgressIndicator *eclipseProgress;
    IBOutlet NSTextField *eclipseReceiver;
    IBOutlet NSButton *findButton;
    BOOL keepGoing;
}
- (IBAction)find:(id)sender;
- (IBAction)go:(id)sender;
- (IBAction)stopFind:(id)sender;

- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
@end
