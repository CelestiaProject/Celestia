//
//  ScriptsController.h
//  celestia
//
//  Created by Da Woon Jung on 2007-05-30.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ScriptsController : NSObject
{
    IBOutlet NSMenu *scriptMenu;
}
- (void)buildScriptMenu;
@end
