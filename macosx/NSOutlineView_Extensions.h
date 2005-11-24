//
//  NSOutlineView_Extensions.h
//
//  Copyright (c) 2001 Apple. All rights reserved.
//


@interface NSOutlineView (MyExtensions)

- (NSArray*)allSelectedItems;
- (void)selectItems:(NSArray*)items byExtendingSelection:(BOOL)extend;

@end

@interface MyOutlineView : NSOutlineView {
}
@end
