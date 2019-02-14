#import "ContextOutlineView.h"

@implementation ContextOutlineView

- (NSMenu*)menuForEvent:(NSEvent*)theEvent
{
    int rowIndex = [self rowAtPoint:[self convertPoint:[theEvent locationInWindow] fromView:nil]];
    if (rowIndex >= 0)
    {
        id item;
        [self abortEditing];
        item = [self itemAtRow:rowIndex];
        if (item != nil) {
            id delegate = [self delegate];
            if ([delegate respondsToSelector:@selector(outlineView:shouldSelectItem:)] && [delegate outlineView:self shouldSelectItem:item])
                [self selectRow:rowIndex byExtendingSelection:NO];
            if ([delegate respondsToSelector:@selector(outlineView:contextMenuForItem:)])
            {
                SEL sel = @selector(outlineView:contextMenuForItem:);
                NSInvocation *invoc = [NSInvocation invocationWithMethodSignature:
                    [delegate methodSignatureForSelector: sel]];
                if (invoc)
                {
                    NSMenu *result;
                    [invoc setSelector: sel];
                    [invoc setArgument: &self atIndex: 2];
                    [invoc setArgument: &item atIndex: 3];
                    [invoc invokeWithTarget: delegate];
                    [invoc getReturnValue: &result];
                    return result;
                }
            }
        }
    } else {
        [self deselectAll:self];
        return [self menu];
    }
    return nil;
}

@end