//
//  CelestiaOpenGLView.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (C) 2007, Celestia Development Team
//
#import "CelestiaOpenGLView.h"
#import "CelestiaAppCore.h"
#import "MacInputWatcher.h"
#import "TextWindowController.h"
#import "Menu_Extensions.h"
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/CGLTypes.h>

#ifndef ATI_FSAA_LEVEL
#define ATI_FSAA_LEVEL  510
#endif

#define CEL_LEFT_BUTTON 1
#define CEL_MIDDLE_BUTTON 2
#define CEL_RIGHT_BUTTON 4


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
    
    glewInit();

    return self;
}

- (void)dealloc
{
    [inputWatcher release];
    [textWindow release];
    [super dealloc];
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

    int auxItemIndex     = -1;
    int separatorIndex   = -1;
    BOOL insertedAuxItem = NO;

    [[self menu] removePlanetarySystemItem];
    [[self menu] removeAltSurfaceItem];
    [[self menu] removeRefMarkItems];

    separatorIndex = [[self menu] numberOfItems] - 4;
    if (separatorIndex < 0) separatorIndex = 0;
    if ([[[self menu] itemAtIndex: separatorIndex] isSeparatorItem])
        ++separatorIndex;
    if ([[[self menu] itemAtIndex: separatorIndex] isSeparatorItem])
        [[self menu] removeItemAtIndex: separatorIndex];

    selection = [[appCore simulation] selection];

    auxItemIndex   = [[self menu] numberOfItems] - 2;
    if (auxItemIndex < 0) auxItemIndex = 0;
    insertedAuxItem =
        [[self menu] insertRefMarkItemsForSelection: selection
                                            atIndex: auxItemIndex];
    auxItemIndex   = [[self menu] numberOfItems] - 2;
    if (auxItemIndex < 0) auxItemIndex = 0;
    insertedAuxItem =
        [[self menu] insertPlanetarySystemItemForSelection: selection
                                                    target: controller
                                                   atIndex: auxItemIndex]
        || insertedAuxItem;
    auxItemIndex   = [[self menu] numberOfItems] - 2;
    if (auxItemIndex < 0) auxItemIndex = 0;
    insertedAuxItem =
        [[self menu] insertAltSurfaceItemForSelection: selection
                                               target: controller
                                              atIndex: auxItemIndex]
        || insertedAuxItem;

    if (insertedAuxItem)
    {
        separatorIndex = [[self menu] numberOfItems] - 2;
        if (separatorIndex > 0)
        {
            [[self menu] insertItem: [NSMenuItem separatorItem]
                            atIndex: separatorIndex];
        }
    }

    if ([selection body])
    {
        selectionName = [[selection body] name];
    }
    else if ([selection star])
    {
        selectionName = [[selection star] name];
    }
    else
    {
        selectionName = [selection briefName];
    }
/*
    selectionName = [[[appCore simulation] selection] briefName];
    if ([selectionName isEqualToString: @""])
    {
        [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers: 0 buttons:CEL_LEFT_BUTTON]];
        [appCore mouseButtonUp:location2 modifiers:[appCore toCelestiaModifiers: 0 buttons:CEL_LEFT_BUTTON]];
        selection = [[appCore simulation] selection];
        selectionName = [[[appCore simulation] selection] name];
    }
*/
    if ([selectionName isEqualToString: @""]) return nil;
    [[[self menu] itemAtIndex: 0] setTitle: selectionName ];
    [[[self menu] itemAtIndex: 0] setEnabled: YES ];
//    [ [self menu] setAutoenablesItems: NO ];
    return [self menu];
}

- (void) textEnterModeChanged
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    if ([appCore textEnterMode] & 1)
    {
        if (textWindow == nil)
            textWindow = [[TextWindowController alloc] init];

        [textWindow makeActiveWithDelegate: inputWatcher];
    }
    else if (0 == ([appCore textEnterMode] & 1))
    {
        [[textWindow window] orderOut: nil];
        [[self window] makeFirstResponder: self];
    }
}

- (void) keyDown: (NSEvent*)theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    if (inputWatcher == nil)
        inputWatcher = [[MacInputWatcher alloc] initWithWatched: self];

    NSString *eventChars = [theEvent characters];
    if (!eventChars || [eventChars length] == 0)
    {
        eventChars = [theEvent charactersIgnoringModifiers];
        if (!eventChars || [eventChars length] == 0)
            return;
    }
    unichar key = [eventChars characterAtIndex: 0];
    int modifiers = [appCore toCelestiaModifiers: [theEvent modifierFlags] buttons: 0];

    if ( key == NSDeleteCharacter )
        key = NSBackspaceCharacter; // delete = backspace
    else if ( key == NSDeleteFunctionKey || key == NSClearLineFunctionKey )
        key = NSDeleteCharacter; // del = delete
    else if ( key == NSBackTabCharacter )
        key = 127;

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
            withModifiers: [appCore toCelestiaModifiers: [theEvent modifierFlags] buttons: 0] ];
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
    NSRect bounds = [self bounds];
    if (!NSPointInRect(location, bounds))
    {
        // -ve coords can crash Celestia so clamp to view bounds
        if (location.x < NSMinX(bounds)) location.x = NSMinX(bounds);
        if (location.x > NSMaxX(bounds)) location.x = NSMaxX(bounds);
        if (location.y < NSMinY(bounds)) location.y = NSMinY(bounds);
        if (location.y > NSMaxY(bounds)) location.y = NSMaxY(bounds);
    }
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
    return YES;
}

- (BOOL) becomeFirstResponder
{
    return YES;
}

- (BOOL) resignFirstResponder
{
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
    if ( type != nil )
    {
        CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
        value = [ pb stringForType: NSStringPboardType ];
        if (value != nil )
        {
            if ( [value rangeOfString:@"cel:" options: (NSCaseInsensitiveSearch|NSAnchoredSearch) ].location == 0 )
                [appCore goToUrl: value ];
            else
                [appCore runScript: value ];
        }
        else
        {
            value = [[NSURL URLWithString: (NSString*) [((NSArray*)[ pb propertyListForType: type ]) objectAtIndex: 0 ]] path];
            [controller runScript: value ];
        
            return NO;
            if (value != nil)
            {
                value = [ pb stringForType: NSFilenamesPboardType ];
                if (value != nil )
                {
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
