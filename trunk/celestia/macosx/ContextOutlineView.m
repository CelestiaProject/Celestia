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
                return [delegate outlineView:self contextMenuForItem:item];
        }
    } else {
        [self deselectAll:self];
        return [self menu];
    }
    return nil;
}

@end