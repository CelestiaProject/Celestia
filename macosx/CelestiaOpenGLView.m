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
        nil
    } ;

    NSOpenGLPixelFormat* pixFmt ;
    long swapInterval ;

    self = [super initWithCoder: coder] ;
    pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs] ;

    [self setPixelFormat: pixFmt] ;

    swapInterval = 1 ;

    [[self openGLContext]
        setValues: &swapInterval
        forParameter: NSOpenGLCPSwapInterval ] ;
        
    return self;
}


- (BOOL) isFlipped {return YES;}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent
{
    CelestiaSelection* selection;
    NSString* selectionName;
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: self];
    NSPoint location2 = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];

    [appCore charEntered: 8 ];
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:NULL buttons:CEL_LEFT_BUTTON]];
    [appCore mouseButtonUp:location2 modifiers:[appCore toCelestiaModifiers:NULL buttons:CEL_LEFT_BUTTON]];

    selection = [[appCore simulation] selection];
    selectionName = [[[appCore simulation] selection] briefName];
    if ([selectionName isEqualToString: @""])
    {
        [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:NULL buttons:CEL_LEFT_BUTTON]];
        [appCore mouseButtonUp:location2 modifiers:[appCore toCelestiaModifiers:NULL buttons:CEL_LEFT_BUTTON]];
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
    unichar key = [[theEvent characters] characterAtIndex: 0];

        if (key == 127)
            key = 8; // delete = backspace 
        if ( key == NSDeleteFunctionKey ) 
           key = 127; // del = delete
//        if ( [theEvent modifierFlags] && NSFunctionKeyMask ) NSLog( @"isFunctionKey");
    if ( (key<128) && ((key < '0') || (key>'9') || !([theEvent modifierFlags] & NSNumericPadKeyMask)) )
       [ appCore charEntered: key ];
//    [ appCore keyDown: [appCore toCelestiaKey: theEvent ] ];
        [ appCore keyDown: [appCore toCelestiaKey: theEvent] 
            withModifiers: [appCore toCelestiaModifiers: [theEvent modifierFlags] buttons: 0]  ];
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

/*
- (void) _updateModifierFlags: (NSEvent*)theEvent
{
    register unsigned int	eventFlags = [theEvent modifierFlags];

    _modifierFlags = 0;

    if(eventFlags & NSControlKeyMask)
        _modifierFlags |= GLUT_ACTIVE_CTRL;

    if(eventFlags & NSShiftKeyMask)
        _modifierFlags |= GLUT_ACTIVE_SHIFT;

    if(eventFlags & NSAlternateKeyMask)
        _modifierFlags |= GLUT_ACTIVE_ALT;
}
*/
- (void) mouseDown: (NSEvent*)theEvent
{
[ [self window] makeFirstResponder: self ];
    if ( [theEvent modifierFlags] & NSAlternateKeyMask )
    {
        [self rightMouseDown: theEvent];
        return;
    }
//     NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    {
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
//    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: self];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    NSLog(@"mouseDown: %@",theEvent);
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
    }
}

- (void) mouseUp: (NSEvent*)theEvent
{
    if ( [theEvent modifierFlags] & NSAlternateKeyMask )
    {
        [self rightMouseUp: theEvent];
        return;
    }
    {
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    NSLog(@"mouseUp: %@", theEvent);
    [appCore mouseButtonUp:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
    }
}
- (void) mouseDragged: (NSEvent*) theEvent
{
    if ( [theEvent modifierFlags] & NSAlternateKeyMask )
    {
        [self rightMouseDragged: theEvent];
        return;
    }
    {
//    NSSize delta = [self convertSize:NSMakeSize([theEvent deltaX], [theEvent deltaY]) fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    NSLog(@"mouseDragged: %@",theEvent);
    [appCore mouseMove:NSMakePoint([theEvent deltaX], [theEvent deltaY]) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
//    [appCore mouseMove:NSMakePoint(delta.width, delta.height) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_LEFT_BUTTON]];
    }
}
- (void) rightMouseDown: (NSEvent*)theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    NSLog(@"rightMouseDown: %@",theEvent);
    [appCore mouseButtonDown:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
}
- (void) rightMouseUp: (NSEvent*)theEvent
{
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    NSLog(@"rightMouseUp: %@", theEvent);
    [appCore mouseButtonUp:location modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
}

- (void) rightMouseDragged: (NSEvent*) theEvent
{
    //NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    //NSSize delta = [self convertSize:NSMakeSize([theEvent deltaX], [theEvent deltaY]) fromView: nil];
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
//    NSLog(@"rightMouseDragged: %@",theEvent);
    [appCore mouseMove:NSMakePoint([theEvent deltaX], [theEvent deltaY]) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
//    [appCore mouseMove:NSMakePoint(delta.width, delta.height) modifiers:[appCore toCelestiaModifiers:[theEvent modifierFlags] buttons:CEL_RIGHT_BUTTON]];
}

- (void) scrollWheel: (NSEvent*)theEvent
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    if ( [[theEvent characters] characterAtIndex: 0] < 128 )
       [ appCore charEntered: [[theEvent characters] characterAtIndex: 0] ];
    [ appCore mouseWheel: [theEvent deltaZ]
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
        [self lockFocus];
        oglContext = [self openGLContext];
        if (oglContext != nil) 
        {
            [controller display];
            glFinish();
            [oglContext flushBuffer] ;

        }
        [self unlockFocus];
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
NSLog(@"read paste");
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
            [appCore runScript: value ];
        
            return NO;
            if (value != nil)
            {
                value = [ pb stringForType: NSFilenamesPboardType ];
                if (value != nil )
                {
                    NSLog(value);
                    [appCore runScript: value ];
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
    NSLog(@"dragentered");
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
