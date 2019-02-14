//
//  MacInputWatcher.mm
//  celestia
//
//  Created by Da Woon Jung on 2007-06-15.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import "MacInputWatcher.h"
#import "CelestiaAppCore.h"
#import "CelestiaController.h"
#import "celestiacore.h"

@interface MacInputWatcher(Private)
- (id) watched;
@end


class _MacInputWatcher : public CelestiaWatcher
{
private:
    MacInputWatcher *watcherWrapper;
    
public:
    _MacInputWatcher(CelestiaAppCore *_appCore, MacInputWatcher *_watcherWrapper) :
        CelestiaWatcher(*[_appCore appCore]), watcherWrapper(_watcherWrapper)
    {};
    
    void notifyChange(CelestiaCore *, int flags)
    {
        if ( 0 != (flags & CelestiaCore::TextEnterModeChanged) )
        {
            id watched = [watcherWrapper watched];
            if ([watched respondsToSelector: @selector(textEnterModeChanged)])
                [watched performSelector: @selector(textEnterModeChanged)];
        }
    };
};


@implementation MacInputWatcher

- (id)initWithWatched: (id)aWatched;
{
    self = [super init];
    if (self)
    {
        watcher = new _MacInputWatcher([CelestiaAppCore sharedAppCore], self);
        watched = aWatched;
    }
    return self;
}

- (void)dealloc
{
    if (watcher) delete watcher;
    [super dealloc];
}

- (id) watched
{
    return watched;
}


- (BOOL) control: (NSControl *)aControl textShouldBeginEditing: (NSText *)aTextObject
{
    BOOL hasMarkedText = NO;
    if ([aTextObject respondsToSelector: @selector(hasMarkedText)])
    {
        SEL sel = @selector(hasMarkedText);
        NSInvocation *invoc = [NSInvocation invocationWithMethodSignature:
            [aTextObject methodSignatureForSelector: sel]];
        if (invoc)
        {
            [invoc setSelector:      sel];
            [invoc invokeWithTarget: aTextObject];
            [invoc getReturnValue:   &hasMarkedText];
        }
    }
    
    [[aTextObject window] makeKeyAndOrderFront: nil];

    if (hasMarkedText)
    {
        [[aTextObject window] setAlphaValue: 1.0f];
    }
    else
    {
        [[aTextObject window] setAlphaValue: 0.0f];
    }
    return YES;
}

-(void) stringEntered: (NSString *)aString
{
    [[CelestiaAppCore sharedAppCore] charEntered: aString];
}

- (void) controlTextDidChange: (NSNotification *)aNotification
{
    id fieldEditor = [[aNotification userInfo] objectForKey: @"NSFieldEditor"];
    NSString *changedText = [fieldEditor string];
    [[[aNotification object] window] setAlphaValue: 0.0f];
    [self stringEntered: changedText];
    if ([fieldEditor shouldChangeTextInRange: NSMakeRange(NSNotFound, 0)
                           replacementString: nil])
    {
        [fieldEditor setString: @""];
        // Make sure textShouldBeginEditing gets reinvoked
        [[[aNotification object] window] makeFirstResponder: [aNotification object]];
    }
}

- (BOOL) control: (NSControl *)aControl textView: (NSTextView *)aTextView doCommandBySelector: (SEL)aCommand
{
    CelestiaAppCore *appCore = [CelestiaAppCore sharedAppCore];
    unichar key = 0;
    int modifiers = 0;
    
    if (@selector(insertNewline:) == aCommand)
        key = NSNewlineCharacter;
    else if (@selector(deleteBackward:) == aCommand)
        key = NSBackspaceCharacter;
    else if (@selector(insertTab:) == aCommand)
        key = NSTabCharacter;
    else if (@selector(insertBacktab:) == aCommand)
        key = 127;
    else if (@selector(cancelOperation:) == aCommand)
        key = '\033';
    else
    {
        // Allow zoom/pan etc during console input
        if (@selector(scrollToBeginningOfDocument:) == aCommand)
            key = CelestiaCore::Key_Home;
        else if (@selector(scrollToEndOfDocument:) == aCommand)
            key = CelestiaCore::Key_End;
        else if (@selector(scrollPageUp:) == aCommand)
            key = CelestiaCore::Key_PageUp;
        else if (@selector(scrollPageDown:) == aCommand)
            key = CelestiaCore::Key_PageDown;
        else if (@selector(moveLeft:) == aCommand)
            key = CelestiaCore::Key_Left;
        else if (@selector(moveLeftAndModifySelection:) == aCommand)
        {   key = CelestiaCore::Key_Left;
            modifiers = [appCore toCelestiaModifiers: NSShiftKeyMask buttons: 0]; }
        else if (@selector(moveRight:) == aCommand)
            key = CelestiaCore::Key_Right;
        else if (@selector(moveRightAndModifySelection:) == aCommand)
        {   key = CelestiaCore::Key_Right;
            modifiers = [appCore toCelestiaModifiers: NSShiftKeyMask buttons: 0]; }
        else if (@selector(moveUp:) == aCommand)
            key = CelestiaCore::Key_Up;
        else if (@selector(moveUpAndModifySelection:) == aCommand)
        {   key = CelestiaCore::Key_Up;
            modifiers = [appCore toCelestiaModifiers: NSShiftKeyMask buttons: 0]; }
        else if (@selector(moveDown:) == aCommand)
            key = CelestiaCore::Key_Down;
        else if (@selector(moveDownAndModifySelection:) == aCommand)
        {   key = CelestiaCore::Key_Down;
            modifiers = [appCore toCelestiaModifiers: NSShiftKeyMask buttons: 0]; }
        else
        {
            NSEvent *currEvent = [[aTextView window] currentEvent];
            if (currEvent && [currEvent type] == NSKeyDown)
            {
                NSString *eventChars = [currEvent charactersIgnoringModifiers];
                if (eventChars && [eventChars length])
                {
                    key = [eventChars characterAtIndex: 0];
                    modifiers = [appCore toCelestiaModifiers: [currEvent modifierFlags] buttons: 0];
                }
            }
            else
                return NO;
        }

        [appCore keyDown: key withModifiers: modifiers];
        return YES;
    }
    
    [appCore charEntered: key withModifiers: modifiers];
    return YES;
}
@end
