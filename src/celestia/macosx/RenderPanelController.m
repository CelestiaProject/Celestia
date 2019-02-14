//
//  RenderPanelController.m
//  celestia
//
//  2005-05 Modified substantially by Da Woon Jung
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "RenderPanelController.h"


@implementation RenderPanelController

- (void)awakeFromNib
{
    [self hideRenderPathWarning];
}

- (void)dealloc
{
    if (_renderPathWarningSuper)
    {
        [renderPathCautionIcon release];
        [renderPathWarning release];
    }

    [super dealloc];
}

- (void)displayRenderPathWarning:(NSString *)warning
{
    if (_renderPathWarningSuper)
    {
        [_renderPathWarningSuper addSubview: renderPathCautionIcon];
        [_renderPathWarningSuper addSubview: renderPathWarning];
        [renderPathCautionIcon release];
        [renderPathWarning release];
        _renderPathWarningSuper = nil;
    }

    [renderPathWarning setStringValue: 
        [NSString stringWithFormat:NSLocalizedString(@"Your system does not support\n%@",""), warning]];
}

- (void)hideRenderPathWarning
{
    if (_renderPathWarningSuper == nil)
    {
        _renderPathWarningSuper = [renderPathWarning superview];
        [renderPathCautionIcon retain];
        [renderPathWarning retain];
        [renderPathCautionIcon removeFromSuperview];
        [renderPathWarning removeFromSuperview];
    }
}
@end
