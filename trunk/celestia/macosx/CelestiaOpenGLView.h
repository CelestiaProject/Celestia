//
//  CelestiaOpenGLView.h
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaController.h"

@class MacInputWatcher;
@class TextWindowController;

@interface CelestiaOpenGLView : NSOpenGLView
{
    IBOutlet CelestiaController *controller;
    MacInputWatcher *inputWatcher;
    TextWindowController *textWindow;
}
- (void)setAASamples: (unsigned int)aaSamples;
@end
