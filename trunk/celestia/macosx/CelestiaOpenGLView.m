//
//  CelestiaOpenGLView.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//
#import "CelestiaOpenGLView.h"
#import <OpenGL/gl.h>

@implementation CelestiaOpenGLView
- (void) drawRect: (NSRect) rect
{
        NSOpenGLContext *oglContext;
        [self lockFocus];
        oglContext = [self openGLContext];
        if (oglContext != nil) {
            [oglContext makeCurrentContext];
            [controller display];
            glFinish();
        }
        [self unlockFocus];
}

- (void) update
{
        [controller setDirty];
        [super update];
}

@end
