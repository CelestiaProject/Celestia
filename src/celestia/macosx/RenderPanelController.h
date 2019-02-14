//
//  RenderPanelController.h
//  celestia
//
//  2005-05 Modified substantially by Da Woon Jung
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "NSWindowController_Extensions.h"

@interface RenderPanelController : NSWindowController
{
    IBOutlet NSImageView *renderPathCautionIcon;
    IBOutlet NSTextField *renderPathWarning;

@private
    NSView *_renderPathWarningSuper;
}

- (void)displayRenderPathWarning:(NSString *)warning;
- (void)hideRenderPathWarning;
@end
