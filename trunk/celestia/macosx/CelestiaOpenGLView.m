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

/*
- (int)_mapNativeKeyCodeToGLUTCode: (unichar)unicodeKey isAscii: (BOOL *)isascii
{
    int	asciiKey = 'A';

    *isascii = NO;
    switch(unicodeKey)
    {
        case NSF1FunctionKey:
            asciiKey = GLUT_KEY_F1;
            break;
        case NSF2FunctionKey:
            asciiKey = GLUT_KEY_F2;
            break;
        case NSF3FunctionKey:
            asciiKey = GLUT_KEY_F3;
            break;
        case NSF4FunctionKey:
            asciiKey = GLUT_KEY_F4;
            break;
        case NSF5FunctionKey:
            asciiKey = GLUT_KEY_F5;
            break;
        case NSF6FunctionKey:
            asciiKey = GLUT_KEY_F6;
            break;
        case NSF7FunctionKey:
            asciiKey = GLUT_KEY_F7;
            break;
        case NSF8FunctionKey:
            asciiKey = GLUT_KEY_F8;
            break;
        case NSF9FunctionKey:
            asciiKey = GLUT_KEY_F9;
            break;
        case NSF10FunctionKey:
            asciiKey = GLUT_KEY_F10;
            break;
        case NSF11FunctionKey:
            asciiKey = GLUT_KEY_F11;
            break;
        case NSF12FunctionKey:
            asciiKey = GLUT_KEY_F12;
            break;
        case NSUpArrowFunctionKey:
            asciiKey = GLUT_KEY_UP;
            break;
        case NSDownArrowFunctionKey:
            asciiKey = GLUT_KEY_DOWN;
            break;
        case NSLeftArrowFunctionKey:
            asciiKey = GLUT_KEY_LEFT;
            break;
        case NSRightArrowFunctionKey:
            asciiKey = GLUT_KEY_RIGHT;
            break;
        case NSPageUpFunctionKey:
            asciiKey = GLUT_KEY_PAGE_UP;
            break;
        case NSPageDownFunctionKey:
            asciiKey = GLUT_KEY_PAGE_DOWN;
            break;
        case NSHomeFunctionKey:
            asciiKey = GLUT_KEY_HOME;
            break;
        case NSEndFunctionKey:
            asciiKey = GLUT_KEY_END;
            break;
        case NSInsertFunctionKey:
            asciiKey = GLUT_KEY_INSERT;
            break;
        default:
            if(unicodeKey < 128)
            {
                *isascii = YES;
                asciiKey = (int) (unicodeKey & 0x00FF);
            }
            break;
    }

    return asciiKey;
}
*/

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
    [ appCore addSurfaceMenu: [self menu] ];
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
    if ( (key<128) && ((key < '0') || (key>'9') || !([theEvent modifierFlags] && NSNumericPadKeyMask)) )
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
    NSPoint location = [self convertPoint: [theEvent locationInWindow] fromView: self];
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
    return YES;
}

- (BOOL) becomeFirstResponder
{
    //NSLog(@"Becoming first responder");
    return YES;
}

- (BOOL) acceptsFirstMouse: (NSEvent*)theEvent
{
    return YES;
}

- (void) drawRect: (NSRect) rect
{
        NSApplication* theApp = [NSApplication sharedApplication];        
        NSOpenGLContext *oglContext;
        [self lockFocus];
        oglContext = [self openGLContext];
        if (oglContext != nil) 
        {
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

- (void) writeStringToPasteboard: (NSPasteboard *) pb
{
    NSString *value;
    [pb declareTypes:
        [NSArray arrayWithObject: NSStringPboardType] owner: self];
//        NSLog ( value );
        CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
        value = [ [appCore currentURL] relativeString ];
        [ pb setString: value forType: NSStringPboardType ];
}

- (BOOL) readStringFromPasteboard: (NSPasteboard *) pb
{
    NSString *value;
    NSString *type = [pb availableTypeFromArray:
        [NSArray arrayWithObject: NSStringPboardType]];
    if ( type != nil )
    {
        CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
        value = [ pb stringForType: NSStringPboardType ];
//        NSLog ( value );
        [appCore goToUrl: value ];
        return YES;
    }
    return NO;
}
- (unsigned int) draggingEntered: (id <NSDraggingInfo>) sender
{
    NSPasteboard *pb = [sender draggingPasteboard];
    NSString *type = [pb availableTypeFromArray:
        [NSArray arrayWithObject: NSStringPboardType]];
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
