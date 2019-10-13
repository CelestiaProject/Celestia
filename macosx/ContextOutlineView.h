/* ContextOutlineView */

#import "NSOutlineView_Extensions.h"

@protocol ContextOutlineViewDelegate <NSOutlineViewDelegate>
@optional
- (NSMenu *)outlineView:(NSOutlineView *)outlineView
contextMenuForItem:(id)item;
@end

@interface ContextOutlineView : NSOutlineView {
}
@end
