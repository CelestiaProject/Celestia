#import "FavoritesDrawerController.h"
#import "CelestiaFavorite.h"
#import "CelestiaAppCore.h"

/*
@implementation NSMenuItem(DebuggingAPI)
-(NSString*)description
{
    return [NSString stringWithFormat:@"<MenuItem: 0x%x %@ enabled:%@ target:%@ representedObject:%@>",self,[self title],[self isEnabled]?@"YES":@"NO",[self target],[self representedObject]];
}
@end
*/

@implementation CelestiaFavorite(ViewAPI)
-(NSMenuItem*)menuItem
{
    NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle:[self name] action:@selector(activateFavorite:) keyEquivalent:@""] autorelease];
    return [self setupMenuItem:menuItem];
}
-(NSMenuItem*)setupMenuItem:(NSMenuItem*)menuItem
{
    [menuItem setTarget:[CelestiaAppCore sharedAppCore]];
    [menuItem setAction:@selector(activateFavorite:)];
    [menuItem setTitle:[self name]];
    [menuItem setRepresentedObject:self];
    [menuItem setKeyEquivalent:@""];
    [menuItem setEnabled:YES];
    return menuItem;
}
@end

@implementation CelestiaFavorites(ViewAPI)
-(void)synchronizeMenu:(NSMenu*)menu atIndex:(unsigned)count
{
    NSEnumerator *enumerator = [self objectEnumerator];
    CelestiaFavorite *fav;
    NSMutableArray *menuStack = [NSMutableArray arrayWithCapacity:[self count]];
    //NSLog(@"[CelestiaFavorites synchronizeMenu:%@ atIndex:%d]",menu,count);
    while ((unsigned)[menu numberOfItems]>count) 
        [menu removeItemAtIndex:[menu numberOfItems]-1];
    //NSString *lastParent=@"";
    [menuStack addObject:menu];
    /* FIXME */
    while ((fav = [enumerator nextObject]) != nil) {
        NSMenuItem* menuItem = [fav menuItem];
        if ([[fav parentFolder] isEqualToString:@""]) 
            [menuStack removeObjectsInRange:NSMakeRange(1,[menuStack count]-1)];
        /*if ([fav isFolder]) {
            NSMenu *subMenu = [[[NSMenu alloc] initWithTitle:[fav name]] autorelease];
            [menuItem setSubmenu:subMenu];
            [menuStack addObject:subMenu];
        }*/
        [[menuStack lastObject] addItem:menuItem];
    }
    //NSLog(@"[CelestiaFavorites synchronizeMenu:%@ atIndex:%d]",menu,count);
}
@end

@implementation FavoritesDrawerController
-(void)awakeFromNib
{
    [outlineView setVerticalMotionCanBeginDrag: YES];
    [favoritesMenu setAutoenablesItems:NO];
}
-(IBAction)addNewFavorite:(id)sender
{
    CelestiaFavorites* favs = [[CelestiaAppCore sharedAppCore] favorites];
    //NSLog(@"[FavoritesDrawerController addNewFavorite:%@]",sender);
    [favs addNewFavorite:nil withParentFolder:nil];
    [self outlineView:outlineView editItem:[favs lastObject]];
}
-(IBAction)addNewFolder:(id)sender
{
    CelestiaFavorites* favs = [[CelestiaAppCore sharedAppCore] favorites];
    //NSLog(@"[FavoritesDrawerController addNewFavorite:%@]",sender);
    [favs addNewFolder:@"untitled folder" withParentFolder:nil];
    [self outlineView:outlineView editItem:[favs lastObject]];
}
-(void)synchronizeFavoritesMenu:(CelestiaFavorites*)favs
{
    //NSLog(@"[FavoritesDrawerController synchronizeFavoritesMenu:]");
    [[[CelestiaAppCore sharedAppCore] favorites] synchronizeMenu:favoritesMenu atIndex:3];
    [outlineView reloadData];
}
-(id)outlineView:(NSOutlineView*)olv child:(int)index ofItem:(id)item
{
    id rval;
    //NSLog(@"[FavoritesDrawerController outlineview:%@ child:%d ofItem:%@]",olv,index,item);
    rval = [[[CelestiaAppCore sharedAppCore] favorites] objectAtIndex:index parent:(CelestiaFavorite*)item];
    //NSLog(@"rval = %@",rval);
    return rval;
}
-(BOOL)outlineView:(NSOutlineView*)olv isItemExpandable:(id)item 
{
    //NSLog(@"[FavoritesDrawerController outlineview:%@ itemIsExpandable:%@]",olv,item);
    return [(CelestiaFavorite*)item isFolder];
}
-(int)outlineView:(NSOutlineView *)olv numberOfChildrenOfItem:(id)item 
{
    int rval;
    //NSLog(@"[FavoritesDrawerController outlineview:%@ numberOfChildrenOfItem:%@]",olv,item);
    rval = [[[CelestiaAppCore sharedAppCore] favorites] numberOfChildrenOfItem:(CelestiaFavorite*)item];
    //NSLog(@"rval = %d",rval);
    return rval;
}
-(id)outlineView:(NSOutlineView *)olv objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item 
{
    id objectValue = nil;
    //NSLog(@"[FavoritesDrawerController outlineview:%@ objectValueForTableColumn:%@ byItem:%@]",olv,tableColumn,item);
    if([[tableColumn identifier] isEqualToString: @"NAME"]) {
	objectValue = [(CelestiaFavorite*)item name];
    } else if([[tableColumn identifier] isEqualToString: @"SELECTION"] && ![(CelestiaFavorite*)item isFolder]) {
	objectValue = [(CelestiaFavorite*)item selectionName];
    }
    //NSLog(@"rval = %@",objectValue);
    return objectValue;
}
// Optional method: needed to allow editing.
-(void)outlineView:(NSOutlineView *)olv setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item  
{
    //NSLog(@"[FavoritesDrawerController outlineview:%@ setObjectValue:%@ forTableColumn:%@ byItem:%@]",olv,object,tableColumn,item);
    if([[tableColumn identifier] isEqualToString: @"NAME"]) {
	[(CelestiaFavorite*)item setName: object];
        [[[CelestiaAppCore sharedAppCore] favorites] synchronize];
        [favoriteInfoWindowController updateFavorite:(CelestiaFavorite*)item];

    }
}

// ================================================================
//  NSOutlineView delegate methods.
// ================================================================
- (NSMenu *)outlineView:(NSOutlineView *)olv 
contextMenuForItem:(id)item
{
    CelestiaFavorite* fav = (CelestiaFavorite*)item;
    NSMenu* contextMenu = [favoritesContextMenu copy];
    NSInvocation* editInv = [NSInvocation invocationWithMethodSignature:[[self class] instanceMethodSignatureForSelector:@selector(outlineView:editItem:)]];
    NSInvocation* delInv = [NSInvocation invocationWithMethodSignature:[[self class] instanceMethodSignatureForSelector:@selector(outlineView:deleteItem:)]];
    NSMenuItem* editItem = [contextMenu itemAtIndex:3];
    NSMenuItem* delItem = [contextMenu itemAtIndex:4];
    NSMutableArray* arr = nil;
    
    BOOL multipleItems = NO;
    //NSLog(@"[FavoritesDrawerController outlineView:%@ contextMenuForItem:%@]",olv,item);
    //NSLog(@"row = %d",[olv rowForItem:item]);
    //[olv selectRow:[olv rowForItem:item] byExtendingSelection:NO];
    if ([olv numberOfSelectedRows]>1 || [olv rowForItem:item]!=[olv selectedRow]) {
        id nextItem = nil;
        NSEnumerator* rowEnum = [olv selectedRowEnumerator];
        arr = [NSMutableArray arrayWithCapacity:[olv numberOfSelectedRows]];
        while ((nextItem = [rowEnum nextObject]) != nil) {
            nextItem = [olv itemAtRow:[nextItem intValue]];
            if (nextItem == nil)
                continue;
            if ([item isEqual:nextItem])
                multipleItems = YES;
            [arr addObject:nextItem];
        }
    }
    if (multipleItems)
        item = arr;
    else
        [olv selectRow:[olv rowForItem:item] byExtendingSelection:NO];
    if ([fav isFolder] || multipleItems) {
        [contextMenu removeItemAtIndex:0];
        [contextMenu removeItemAtIndex:0];
        [contextMenu removeItemAtIndex:0];
    } else {
        NSMenuItem* navItem = [contextMenu itemAtIndex:0];
        NSString* title = [navItem title];
        NSMenuItem* showItem = [contextMenu itemAtIndex:1];
        [showItem setTarget:favoriteInfoWindowController];
        [showItem setAction:@selector(showWindow:)];
        [fav setupMenuItem:navItem];
        [navItem setTitle:title];
    }
    if (multipleItems) {
        [contextMenu removeItemAtIndex:0];
    } else {
        [editInv setTarget:self];
        [editInv setSelector:@selector(outlineView:editItem:)];
        [editInv setArgument:&olv atIndex:2];
        [editInv setArgument:&item atIndex:3];
        [editItem setTarget:editInv];
        [editItem setAction:@selector(invoke)];
    }
    [delInv setTarget:self];
    [delInv setSelector:@selector(outlineView:deleteItem:)];
    [delInv setArgument:&olv atIndex:2];
    [delInv setArgument:&item atIndex:3];
    [delItem setTarget:delInv];
    [delItem setAction:@selector(invoke)];
    return contextMenu;
}
- (void)outlineView:(NSOutlineView*)olv editItem:(CelestiaFavorite*)item
{
    int row = [olv rowForItem:item];
    //NSLog(@"[FavoritesDrawerController outlineView:%@ editItem:%@]",olv,item);
    //NSLog(@"row = %d",row); // -1 ??
    [olv selectRow:row byExtendingSelection:NO];
    [olv editColumn:[olv columnWithIdentifier:@"NAME"] row:row withEvent:nil select:YES];
}

- (void)outlineView:(NSOutlineView*)olv deleteItem:(id)item
{
    //NSLog(@"[FavoritesDrawerController outlineView:%@ deleteItem:%@]",olv,item);
    CelestiaFavorites *favs = [[CelestiaAppCore sharedAppCore] favorites];
    if ([item isKindOfClass:[CelestiaFavorite class]])
        [favs removeObject:(CelestiaFavorite*)item];
    else
        [favs removeObjectsInArray:(NSArray*)item];
    [olv deselectAll:self];
    [favoriteInfoWindowController updateFavorite:nil];
}

-(void)outlineViewSelectionDidChange:(NSNotification*)notification
{
    NSOutlineView* olv = [notification object];
    if ([olv numberOfSelectedRows]==1) {
        [favoriteInfoWindowController updateFavorite:[olv itemAtRow:[olv selectedRow]]];
    } else {
        [favoriteInfoWindowController updateFavorite:nil];
    }
}
/*
- (BOOL)outlineView:(NSOutlineView *)olv shouldExpandItem:(id)item 
{
    BOOL rval;
    NSLog(@"[FavoritesDrawerController outlineview:%@ shouldExpandItem:%@]",olv,item);
    rval = [(CelestiaFavorite*)item isFolder];
    NSLog(@"rval = %@",((rval)?@"YES":@"NO"));
    return rval;
}

- (void)outlineView:(NSOutlineView *)olv willDisplayCell:(NSCell *)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item 
{    
    // NSLog(@"[FavoritesDrawerController outlineview:%@ willDisplayCell:%@ forTableColumn:%@ item:0x%X]",olv,cell,tableColumn,item);
    if ([[tableColumn identifier] isEqualToString: @"NAME"]) {
	[cell setStringValue: [(CelestiaFavorite*)item name]];
    } else if ([[tableColumn identifier] isEqualToString: @"SELECTION"]) {
	if ([(CelestiaFavorite*)item isFolder])
            [cell setStringValue:@""];
        else
            [cell setStringValue:[(CelestiaFavorite*)item selectionName]];
    }
}
*/

// ================================================================
//  NSOutlineView data source methods. (dragging related)
// ================================================================
/*
- (BOOL)outlineView:(NSOutlineView *)olv writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard 
{
    draggedNodes = items; // Don't retain since this is just holding temporaral drag information, and it is only used during a drag!  We could put this in the pboard actually.
    
    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:[NSArray arrayWithObjects: DragDropSimplePboardType, NSStringPboardType, nil] owner:self];
    
    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:DragDropSimplePboardType]; 
    
    // Put string data on the pboard... notice you candrag into TextEdit!
    [pboard setString: [draggedNodes description] forType: NSStringPboardType];

    return YES;
}

- (unsigned int)outlineView:(NSOutlineView*)olv validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(int)childIndex {
    // This method validates whether or not the proposal is a valid one. Returns NO if the drop should not be allowed.
    SimpleTreeNode *targetNode = item;
    BOOL targetNodeIsValid = YES;

    if ([self onlyAcceptDropOnRoot]) {
	targetNode = nil;
	childIndex = NSOutlineViewDropOnItemIndex;
    } else {
	BOOL isOnDropTypeProposal = childIndex==NSOutlineViewDropOnItemIndex;
	
	// Refuse if: dropping "on" the view itself unless we have no data in the view.
	if (targetNode==nil && childIndex==NSOutlineViewDropOnItemIndex && [treeData numberOfChildren]!=0) 
	    targetNodeIsValid = NO;
    
	if (targetNode==nil && childIndex==NSOutlineViewDropOnItemIndex && [self allowOnDropOnLeaf]==NO)
	    targetNodeIsValid = NO;
	
	// Refuse if: we are trying to do something which is not allowed as specified by the UI check boxes.
	if (targetNodeIsValid && isOnDropTypeProposal==NO && [self allowBetweenDrop]==NO ||
	    [NODE_DATA(targetNode) isGroup] && isOnDropTypeProposal==YES && [self allowOnDropOnGroup]==NO ||
	    [NODE_DATA(targetNode) isLeaf ] && isOnDropTypeProposal==YES && [self allowOnDropOnLeaf]==NO)
	    targetNodeIsValid = NO;
	    
	// Check to make sure we don't allow a node to be inserted into one of its descendants!
	if (targetNodeIsValid && ([info draggingSource]==outlineView) && [[info draggingPasteboard] availableTypeFromArray:[NSArray arrayWithObject: DragDropSimplePboardType]] != nil) {
	    NSArray *_draggedNodes = [[[info draggingSource] dataSource] draggedNodes];
	    targetNodeIsValid = ![targetNode isDescendantOfNodeInArray: _draggedNodes];
	}
    }
    
    // Set the item and child index in case we computed a retargeted one.
    [outlineView setDropItem:targetNode dropChildIndex:childIndex];
    
    return targetNodeIsValid ? NSDragOperationGeneric : NSDragOperationNone;
}

- (void)_performDropOperation:(id <NSDraggingInfo>)info onNode:(TreeNode*)parentNode atIndex:(int)childIndex {
    // Helper method to insert dropped data into the model. 
    NSPasteboard * pboard = [info draggingPasteboard];
    NSMutableArray * itemsToSelect = nil;
    
    // Do the appropriate thing depending on wether the data is DragDropSimplePboardType or NSStringPboardType.
    if ([pboard availableTypeFromArray:[NSArray arrayWithObjects:DragDropSimplePboardType, nil]] != nil) {
        AppController *dragDataSource = [[info draggingSource] dataSource];
        NSArray *_draggedNodes = [TreeNode minimumNodeCoverFromNodesInArray: [dragDataSource draggedNodes]];
        NSEnumerator *draggedNodesEnum = [_draggedNodes objectEnumerator];
        SimpleTreeNode *_draggedNode = nil, *_draggedNodeParent = nil;
        
	itemsToSelect = [NSMutableArray arrayWithArray:[self selectedNodes]];
	
        while ((_draggedNode = [draggedNodesEnum nextObject])) {
            _draggedNodeParent = (SimpleTreeNode*)[_draggedNode nodeParent];
            if (parentNode==_draggedNodeParent && [parentNode indexOfChild: _draggedNode]<childIndex) childIndex--;
            [_draggedNodeParent removeChild: _draggedNode];
        }
        [parentNode insertChildren: _draggedNodes atIndex: childIndex];
    } 
    else if ([pboard availableTypeFromArray:[NSArray arrayWithObject: NSStringPboardType]]) {
        NSString *string = [pboard stringForType: NSStringPboardType];
	SimpleTreeNode *newItem = [SimpleTreeNode treeNodeWithData: [SimpleNodeData leafDataWithName:string]];
	
	itemsToSelect = [NSMutableArray arrayWithObject: newItem];
	[parentNode insertChild: newItem atIndex:childIndex++];
    }

    [outlineView reloadData];
    [outlineView selectItems: itemsToSelect byExtendingSelection: NO];
}

- (BOOL)outlineView:(NSOutlineView*)olv acceptDrop:(id <NSDraggingInfo>)info item:(id)targetItem childIndex:(int)childIndex {
    TreeNode * 		parentNode = nil;
    
    // Determine the parent to insert into and the child index to insert at.
    if ([NODE_DATA(targetItem) isLeaf]) {
        parentNode = (SimpleTreeNode*)(childIndex==NSOutlineViewDropOnItemIndex ? [targetItem nodeParent] : targetItem);
        childIndex = (childIndex==NSOutlineViewDropOnItemIndex ? [[targetItem nodeParent] indexOfChild: targetItem]+1 : 0);
        if ([NODE_DATA(parentNode) isLeaf]) [NODE_DATA(parentNode) setIsLeaf:NO];
    } else {            
        parentNode = SAFENODE(targetItem);
	childIndex = (childIndex==NSOutlineViewDropOnItemIndex?0:childIndex);
    }
    
    [self _performDropOperation:info onNode:parentNode atIndex:childIndex];
        
    return YES;
}
*/
@end
