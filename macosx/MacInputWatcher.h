//
//  MacInputWatcher.h
//  celestia
//
//  Created by Da Woon Jung on 2007-06-15.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class _MacInputWatcher;


@interface MacInputWatcher : NSObject
{
    _MacInputWatcher *watcher;
    id watched;
}
- (id)initWithWatched: (id)aWatched;

- (BOOL) control: (NSControl *)aControl textShouldBeginEditing: (NSText *)aTextObject;
- (void) controlTextDidChange: (NSNotification *)aNotification;
- (BOOL) control: (NSControl *)aControl textView: (NSTextView *)aTextView doCommandBySelector: (SEL)aCommand;
- (void) stringEntered: (NSString *)aString;
@end
