//
//  CelestiaOpenGLView.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//
#import "CelestiaOpenGLView.h"
#import "CelestiaAppCore.h"
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/CGLTypes.h>

#ifndef ATI_FSAA_LEVEL
#define ATI_FSAA_LEVEL  510
#endif

#define CEL_LEFT_BUTTON 1
#define CEL_MIDDLE_BUTTON 2
#define CEL_RIGHT_BUTTON 4

//static CelestiaAppCore* appCore = nil;
//static NSPoint lastLoc = NSMakePoint(0.0, 0.0);

@implementation CelestiaOpenGLView

- (id) initWithCoder: (NSCoder *) coder
{
    NSOpenGLPixelFormatAttribute attrs[] = 
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize,
        (NSOpenGLPixelFormatAttribute)32,
        nil
    } ;

    NSOpenGLPixelFormat* pixFmt ;
    long swapInterval ;

    self = [super initWithCoder: coder] ;
    if (self)
    {
        pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs];

        if (pixFmt)
        {
            [self setPixelFormat: pixFmt] ;
            [pixFmt release];

            if (0 == CGLEnable([[self openGLContext] CGLContextObj], 313))
            {
                NSLog(@"Multithreaded OpenGL enabled.");
            }
            else
            {
                NSLog(@"Multithreaded OpenGL not supported on your system.");
            }

            swapInterval = 1 ;

            [[self openGLContext]
                setValues: &swapInterval
                forParameter: NSOpenGLCPSwapInterval ] ;
        }
        else
        {
            [self release];
            return nil;
        }
    }

    return self;
}

- (void)setAASamples: (unsigned int)aaSamples
{
    if (aaSamples > 1)
    {
        const char *glRenderer = (const char *) glGetString(GL_RENDERER);

        if (strstr(glRenderer, "ATI"))
        {
            [[self openGLContext] setValues: (const long *)&aaSamples
                               forParameter: ATI_FSAA_LEVEL];
        }
        else
        {
            NSOpenGLPixelFormat *pixFmt;
            NSOpenGLContext *context;

            NSOpenGLPixelFormatAttribute fsaaAttrs[] =
            {
                NSOpenGLPFADoubleBuffer,
                NSOpenGLPFADepthSize,
                (NSOpenGLPixelFormatAttribute)32,
                NSOpenGLPFASampleBuffers,
                (NSOpenGLPixelFormatAttribute)1,
                NSOpenGLPFASamples,
                (NSOpenGLPixelFormatAttribute)1,
                nil
            };

            fsaaAttrs[6] = aaSamples;

            pixFmt =
                [[NSOpenGLPixelFormat alloc] initWithAttributes: fsaaAttrs];

            if (pixFmt)
            {
                context = [[NSOpenGLContext alloc] initWithFormat: pixFmt
                                                     shareContext: nil];
                [pixFmt release];

                if (context)
                {
                    // The following silently fails if not supported
                    CGLEnable([context CGLContextObj], 313);

                    long swapInterval = 1;
                    [context setValues: &swapInterval
                          forParameter: NSOpenGLCPSwapInterval];
                    [self setOpenGLContext: context];
                    [context setView: self];
                    [context makeCurrentContext];
                    [context release];

                    glEnable(GL_MULTISAMPLE_ARB);
                    // GL_NICEST enables Quincunx on supported NVIDIA cards,
                    // but smears text.
//                    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
                }
            }
        }
    }
}


- (BOOL) isFlipped {return YES;}

- (void) viewDidMoveToWindow
{
    [[self window] setAcceptsMouseMovedEvents: YES];
}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
    CelestiaSelection* selection;
    NSString* selectionName;
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: self];
    NSPoint location2 = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];

    [appCore charEntered: 8 withModifiers:0];
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers: 0 buttons:CEL_LEFT_BUTTON]];
    [appCore mouseButtonUp:location2 modifiers:[appCore toCelestiaModifiers: 0 buttons:CEL_LEFT_BUTTON]];

    selection = [[appCore simulation] selection];
    selectionName = [[[appCore simulation] selection] briefName];
    if ([selectionName isEqualToString: @""])
    {
        [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers: 0 buttons:CEL_LEFT_BUTTON]];
        [appCore mouseButtonUp:location2 modifiers:[appCore toCelestiaModifiers: 0 buttons:CEL_LEFT_BUTTON]];
        selection = [[appCore simulation] selection];
        selectionName = [[[appCore simulation] selection] name];
    } 
    if ([selectionName isEqualToString: @""]) return NULL;
    [[[self menu] itemAtIndex: 0] setTitle: selectionName ];
    [[[self menu] itemAtIndex: 0] setEnabled: YES ];
    [ controller addSurfaceMenu: [self menu] ];
//    [ [self menu] setAutoenablesItems: NO ];
    return [self menu];
}

- (void) keyDown: (NSEvent*)theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    NSString *eventChars = [theEvent characters];
    if (!eventChars || [eventChars length] == 0)
    {
        eventChars = [theEvent charactersIgnoringModifiers];
        if (!eventChars || [eventChars length] == 0)
            return;
    }
    unichar key = [eventChars characterAtIndex: 0];
    int modifiers = [appCore toCelestiaModifiers: [theEvent modifierFlags] buttons: 0];

        if (key == 127)
        key = 8; // delete = backspace
    else if ( key == NSDeleteFunctionKey || key == NSClearLineFunctionKey )
           key = 127; // del = delete
    
    if ( (key<128) && ((key < '0') || (key>'9') || !([theEvent modifierFlags] & NSNumericPadKeyMask)) )
        [ appCore charEntered: key
                withModifiers: modifiers];

        [ appCore keyDown: [appCore toCelestiaKey: theEvent] 
        withModifiers: modifiers  ];
}

- (void) keyUp: (NSEvent*)theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    if ( [[theEvent characters] characterAtIndex: 0] >= 128 )
//        [ appCore keyUp: [appCore toCelestiaKey: theEvent] ];
        [ appCore keyUp: [appCore toCelestiaKey: theEvent] 
            withModifiers: [appCore toCelestiaModifiers: [theEvent modifierFlags] buttons: 0]  ];
//    NSLog(@"keyUp: %@",theEvent);
}

- (void) mouseDown: (NSEvent*)theEvent
{
    [ [self window] makeFirstResponder: self ];

     NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
}

- (void) mouseUp: (NSEvent*)theEvent
{
    if ( [theEvent modifierFlags] & NSAlternateKeyMask )
    {
        [self otherMouseUp: theEvent];
        return;
    }

    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseButtonUp:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
}

- (void) mouseDragged: (NSEvent*) theEvent
{
    if ( [theEvent modifierFlags] & NSAlternateKeyMask )
    {
        [self rightMouseDragged: theEvent];
        return;
    }

    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseMove:NSMakePoint([theEvent deltaX], [theEvent deltaY]) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
}

- (void) mouseMoved: (NSEvent*)theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    [appCore mouseMove: location];
}

- (void) rightMouseDown: (NSEvent*)theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
}

- (void) rightMouseUp: (NSEvent*)theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseButtonUp:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
    if([theEvent clickCount] > 0)
        [super rightMouseDown:theEvent];    //...Force context menu to appear only on clicks (not drags)
}

- (void) rightMouseDragged: (NSEvent*) theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseMove:NSMakePoint([theEvent deltaX], [theEvent deltaY]) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
}

- (void) otherMouseDown: (NSEvent *) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_MIDDLE_BUTTON]];
}

- (void) otherMouseUp: (NSEvent *) theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseButtonUp:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_MIDDLE_BUTTON]];    
}

- (void) otherMouseDragged: (NSEvent *) theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [appCore mouseMove:NSMakePoint([theEvent deltaX], [theEvent deltaY]) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_MIDDLE_BUTTON]];
}

- (void) scrollWheel: (NSEvent*)theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    [ appCore mouseWheel: [theEvent deltaY]
       modifiers: [appCore toCelestiaModifiers: [theEvent modifierFlags] buttons: 0] ];
}

- (BOOL) acceptsFirstResponder: (NSEvent*)theEvent
{
//    NSLog(@"CelestiaOpenGLView acceptsFirstResponder" );
    return YES;
}

- (BOOL) becomeFirstResponder
{
//    NSLog(@"CelestiaOpenGLView becomeFirstResponder" );
    return YES;
}

- (BOOL) resignFirstResponder
{
//    NSLog(@"CelestiaOpenGLView resignFirstResponder" );
    return YES;
}

- (BOOL) acceptsFirstMouse: (NSEvent*)theEvent
{
    return YES;
}

- (void) drawRect: (NSRect) rect
{
    NSOpenGLContext *oglContext;
    oglContext = [self openGLContext];
    if (oglContext != nil) 
    {
        [controller display];
        [oglContext flushBuffer];
    }
}

- (void) update
{
    [controller setDirty];
    [super update];
}

- (void) writeStringToPasteboard: (NSPasteboard *) pb
{
    NSString *value;
    [pb declareTypes:
        [NSArray arrayWithObject: NSStringPboardType] owner: self];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    value = [appCore currentURL];
    [ pb setString: value forType: NSStringPboardType ];
}

- (BOOL) readStringFromPasteboard: (NSPasteboard *) pb
{
    NSString *value = nil;
    NSString *type = [pb availableTypeFromArray:
        [NSArray arrayWithObjects: NSURLPboardType, NSFilenamesPboardType, NSStringPboardType, nil ]];
//NSLog(@"read paste");
    if ( type != nil )
    {
        CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//        value = [ pb stringForType: NSURLPboardType ];
          value = [ pb stringForType: NSStringPboardType ];
        if (value != nil )
        {
            NSLog(value);
            if ( [value rangeOfString:@"cel:" options: (NSCaseInsensitiveSearch|NSAnchoredSearch) ].location == 0 )
                [appCore goToUrl: value ];
            else
                [appCore runScript: value ];
        }
        else
        {
            value = [[NSURL URLWithString: (NSString*) [((NSArray*)[ pb propertyListForType: type ]) objectAtIndex: 0 ]] path];
            NSLog(value);
            [controller runScript: value ];
        
            return NO;
            if (value != nil)
            {
                value = [ pb stringForType: NSFilenamesPboardType ];
                if (value != nil )
                {
                    NSLog(value);
                    [controller runScript: value ];
                }
            }
        }
        return YES;
    }
    return NO;
}
- (unsigned int) draggingEntered: (id <NSDraggingInfo>) sender
{
    NSPasteboard *pb = [sender draggingPasteboard];
    NSString *type = [pb availableTypeFromArray:
        [NSArray arrayWithObjects: NSStringPboardType, NSFilenamesPboardType, nil ]];
//    NSLog(@"dragentered");
    if ( type != nil )
    {
        return NSDragOperationCopy;
    }
    return NSDragOperationNone;
}

- (BOOL) prepareForDragOperation: (id <NSDraggingInfo>) sender
{
    return YES;
}

- (BOOL) performDragOperation: (id <NSDraggingInfo>) sender
{
    NSPasteboard *pb = [sender draggingPasteboard];
    return [ self readStringFromPasteboard: pb ];
}

- (IBAction) paste: (id) sender
{
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [ self readStringFromPasteboard: pb ];
}

- (IBAction) copy: (id) sender
{
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [ self writeStringToPasteboard: pb ];
}

@end
