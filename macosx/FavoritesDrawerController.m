#import "FavoritesDrawerController.h"
#import "CelestiaFavorite.h"
#import "CelestiaFavorites.h"
#define SAFENODE(node) ((MyTree*)((node == nil) ? [CelestiaFavorites sharedFavorites] : node))
#define DragDropSimplePboardType @"CelestiaFavoriteOutlineViewPboardType"
/*
@implementation NSMenuItem(DebuggingAPI)
-(NSString*)description
{
    return [NSString stringWithFormat:@"<MenuItem: 0x%x %@ enabled:%@ target:%@ representedObject:%@>",self,[self title],[self isEnabled]?@"YES":@"NO",[self target],[self representedObject]];
}
@end
*/
@implementation CelestiaFavorite(ViewAPI)
-(NSMenuItem*)favoriteMenuItem
{
    NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle:[self name] action:([self isFolder] ? nil : @selector(activate)) keyEquivalent:@""] autorelease];
    return [self setupFavoriteMenuItem:menuItem];
}
-(NSMenuItem*)setupFavoriteMenuItem:(NSMenuItem*)menuItem
{
    [menuItem setTarget:self];
    [menuItem setAction:([self isFolder] ? nil : @selector(activate))];
    [menuItem setTitle:[self name]];
    [menuItem setRepresentedObject:self];
    [menuItem setKeyEquivalent:@""];
    [menuItem setEnabled:YES];
    return menuItem;
}
@end

@implementation CelestiaFavorites(ViewAPI)
-(NSArray*)favoriteMenuItems
{
    NSEnumerator *enumerator = [[self children] objectEnumerator];
    MyTree* node = nil;
    NSMutableArray* menuItems = [NSMutableArray arrayWithCapacity:[[self children] count]];
    //NSLog(@"[CelestiaFavorites favoriteMenuItems]");
    while ((node = [enumerator nextObject]) != nil)
        [menuItems addObject:[node favoriteMenuItem]];
    return (NSArray*)menuItems;
}
@end
@implementation MyTree(ViewAPI)
-(void)activate
{
    [(CelestiaFavorite*)[self nodeValue] activate];
}
-(NSMenuItem*)favoriteMenuItem
{
    NSEnumerator* enumerator = nil;
    NSMenu* subMenu = nil;
    NSMenuItem* menuItem = [[self nodeValue] favoriteMenuItem];
    MyTree* node = nil;
    //NSLog(@"[MyTree favoriteMenuItem]");
    if ([self isLeaf])
        return menuItem;
    enumerator = [[self children] objectEnumerator];
    subMenu = [[[NSMenu alloc] initWithTitle:[[self nodeValue] name]] autorelease];
    while ((node = [enumerator nextObject]) != nil)
        [subMenu addItem:[node favoriteMenuItem]];
    [menuItem setSubmenu:subMenu];
    [menuItem setTarget:menuItem];
    [menuItem setAction:@selector(submenuAction:)];
    return menuItem;
}
@end

@implementation FavoritesDrawerController
-(NSArray*)selectedNodes
{
    return [outlineView allSelectedItems];
}
-(NSArray*)draggedNodes
{
    return draggedNodes;
}
-(void)activateFavorite:(CelestiaFavorite*)fav
{
    id menuItem = fav;
    if ([menuItem isKindOfClass:[NSMenuItem class]])
        fav = [(NSMenuItem*)menuItem representedObject];
    [fav activate];
    [outlineView deselectAll:self];
}
-(void)awakeFromNib
{
    draggedNodes = nil;
    [outlineView setVerticalMotionCanBeginDrag: YES];
    [outlineView setTarget:self];
    [outlineView setDoubleAction:@selector(doubleClick:)];
    [outlineView registerForDraggedTypes:[NSArray arrayWithObjects:DragDropSimplePboardType, nil]];
    [favoritesMenu setAutoenablesItems:NO];
}
-(IBAction)addNewFavorite:(id)sender
{
    CelestiaFavorites* favs = [CelestiaFavorites sharedFavorites];
    MyTree* node = nil;
    //NSLog(@"[FavoritesDrawerController addNewFavorite:%@]",sender);
    node = [favs addNewFavorite:nil];

    // kludge to fix name
    {
    CelestiaAppCore* appCore = [CelestiaAppCore sharedAppCore];
    NSString* newName  = [ [ [appCore simulation] selection ] briefName ];
    [((CelestiaFavorite*)[node nodeValue]) setName: newName];
    }

    [[CelestiaFavorites sharedFavorites] synchronize];
//    [self outlineView:outlineView editItem:node];
}
-(IBAction)addNewFolder:(id)sender
{
    CelestiaFavorites* favs = [CelestiaFavorites sharedFavorites];
    MyTree* node = nil;
    //NSLog(@"[FavoritesDrawerController addNewFavorite:%@]",sender);
    node = [favs addNewFolder:NSLocalizedString(@"untitled folder",@"")];
    [[CelestiaFavorites sharedFavorites] synchronize];
    [self outlineView:outlineView editItem:node];
}
-(IBAction)doubleClick:(id)sender
{
    //NSLog(@"[FavoritesDrawerController doubleClick:%@]",sender);
    if ([outlineView numberOfSelectedRows]==1)
    {
        // Make sure item is not a folder
        id theItem = [outlineView itemAtRow:[outlineView selectedRow]];
        if (![outlineView isExpandable: theItem])
            [self activateFavorite:[theItem nodeValue]];
    }
}
-(void)synchronizeFavoritesMenu
{
    NSEnumerator *enumerator = [[[CelestiaFavorites sharedFavorites] favoriteMenuItems] objectEnumerator];
    NSMenuItem *menuItem = nil;
    // remove old menu items
    while ([favoritesMenu numberOfItems]>3)
        [favoritesMenu removeItemAtIndex:[favoritesMenu numberOfItems]-1];
    while ((menuItem = [enumerator nextObject]) != nil)
        [favoritesMenu addItem:menuItem];
    [outlineView reloadData];
}
-(id)outlineView:(NSOutlineView*)olv child:(int)index ofItem:(id)item
{
    id rval;
    //NSLog(@"[FavoritesDrawerController outlineview:%@ child:%d ofItem:%@]",olv,index,item);
    rval = [SAFENODE(item) childAtIndex:index];
    //NSLog(@"rval = %@",rval);
    return rval;
}
-(BOOL)outlineView:(NSOutlineView*)olv isItemExpandable:(id)item 
{
    //NSLog(@"[FavoritesDrawerController outlineview:%@ itemIsExpandable:%@]",olv,item);
    return ![SAFENODE(item) isLeaf];
}
-(int)outlineView:(NSOutlineView *)olv numberOfChildrenOfItem:(id)item 
{
    int rval;
    //NSLog(@"[FavoritesDrawerController outlineview:%@ numberOfChildrenOfItem:%@]",olv,item);
    rval = [SAFENODE(item) numberOfChildren];
    //NSLog(@"rval = %d",rval);
    return rval;
}
-(id)outlineView:(NSOutlineView *)olv objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item 
{
    id objectValue = nil;
    //NSLog(@"[FavoritesDrawerController outlineview:%@ objectValueForTableColumn:%@ byItem:%@]",olv,tableColumn,item);
    if([[tableColumn identifier] isEqualToString: @"NAME"]) {
	objectValue = [(CelestiaFavorite*)[SAFENODE(item) nodeValue] name];
    } else if([[tableColumn identifier] isEqualToString: @"SELECTION"] && [SAFENODE(item) isLeaf]) {
	objectValue = [(CelestiaFavorite*)[SAFENODE(item) nodeValue] selectionName];
    }
    //NSLog(@"rval = %@",objectValue);
    return objectValue;
}
// Optional method: needed to allow editing.
-(void)outlineView:(NSOutlineView *)olv setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item  
{
    //NSLog(@"[FavoritesDrawerController outlineview:%@ setObjectValue:%@ forTableColumn:%@ byItem:%@]",olv,object,tableColumn,item);
    if([[tableColumn identifier] isEqualToString: @"NAME"]) {
        [(CelestiaFavorite*)[SAFENODE(item) nodeValue] setName: object];
        [[CelestiaFavorites sharedFavorites] synchronize];
        [(FavoriteInfoWindowController*)favoriteInfoWindowController updateFavorite:(CelestiaFavorite*)[SAFENODE(item) nodeValue]];

    }
}

// ================================================================
//  NSOutlineView delegate methods.
// ================================================================
- (NSMenu *)outlineView:(NSOutlineView *)olv 
contextMenuForItem:(id)item
{
    MyTree* node = SAFENODE(item);
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
    if (![node isLeaf] || multipleItems) {
        [contextMenu removeItemAtIndex:0];
        [contextMenu removeItemAtIndex:0];
        [contextMenu removeItemAtIndex:0];
    } else {
        NSMenuItem* navItem = [contextMenu itemAtIndex:0];
        NSString* title = [navItem title];
        NSMenuItem* showItem = [contextMenu itemAtIndex:1];
        [showItem setTarget:favoriteInfoWindowController];
        [showItem setAction:@selector(showWindow:)];
        [(CelestiaFavorite*)[node nodeValue] setupFavoriteMenuItem:navItem];
        [navItem setTarget:self];
        [navItem setTitle:title];
        [navItem setAction:@selector(activateFavorite:)];
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
- (void)outlineView:(NSOutlineView*)olv editItem:(id)item
{
    int row = [olv rowForItem:item];
    //NSLog(@"[FavoritesDrawerController outlineView:%@ editItem:%@]",olv,item);
    //NSLog(@"row = %d",row); // -1 ??
    if (row<0) {
        NSLog(@"row is -1");
        return;
    }
    [olv selectRow:row byExtendingSelection:NO];
    [olv editColumn:[olv columnWithIdentifier:@"NAME"] row:row withEvent:nil select:YES];
}

- (void)outlineView:(NSOutlineView*)olv deleteItem:(id)item
{
    //NSLog(@"[FavoritesDrawerController outlineView:%@ deleteItem:%@]",olv,item);
    if ([item isKindOfClass:[MyTree class]])
        [item removeFromParent];
    else
        [(NSArray*)item makeObjectsPerformSelector:@selector(removeFromParent)];
    [[CelestiaFavorites sharedFavorites] synchronize];
    [olv deselectAll:self];
    [(FavoriteInfoWindowController*)favoriteInfoWindowController updateFavorite:nil];
}

-(void)outlineViewSelectionDidChange:(NSNotification*)notification
{
    NSOutlineView* olv = [notification object];
    if ([olv numberOfSelectedRows]==1) {
        [(FavoriteInfoWindowController*)favoriteInfoWindowController updateFavorite:[[olv itemAtRow:[olv selectedRow]] nodeValue]];
    } else {
        [(FavoriteInfoWindowController*)favoriteInfoWindowController updateFavorite:nil];
    }
}
/*
- (BOOL)outlineView:(NSOutlineView *)olv shouldExpandItem:(id)item 
{
    BOOL rval;
    NSLog(@"[FavoritesDrawerController outlineview:%@ shouldExpandItem:%@]",olv,item);
    rval = ![SAFENODE(item) isLeaf];
    NSLog(@"rval = %@",((rval)?@"YES":@"NO"));
    return rval;
}
*/
/*
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

- (BOOL)outlineView:(NSOutlineView *)olv writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard 
{
    //NSLog(@"[FavoritesDrawerController outlineView:%@ writeItems:%@ toPasteboard:%@]",olv,items,pboard);
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
    MyTree *targetNode = item;
    BOOL targetNodeIsValid = YES;

    BOOL isOnDropTypeProposal = childIndex==NSOutlineViewDropOnItemIndex;
    //NSLog(@"[FavoritesDrawerController outlineView:%@ validateDrop:%@ proposedItem:%@ proposedChildIndex:%d]",olv,info,item,childIndex);
    // Refuse if: dropping "on" the view itself unless we have no data in the view.
    /*
     if (targetNode==nil && isOnDropTypeProposal==YES && [self outlineView:olv numberOfChildrenOfItem:nil]!=0) 
        targetNodeIsValid = NO;
    */
    if (targetNode==nil && isOnDropTypeProposal==YES)// && [self allowOnDropOnLeaf]==NO)
        targetNodeIsValid = NO;

    if (targetNodeIsValid && [targetNode isLeaf])
        targetNodeIsValid = NO;

    if (isOnDropTypeProposal)
        targetNodeIsValid = NO;
    
    // Check to make sure we don't allow a node to be inserted into one of its descendants!
    if (targetNodeIsValid && ([info draggingSource]==outlineView) && [[info draggingPasteboard] availableTypeFromArray:[NSArray arrayWithObject: DragDropSimplePboardType]] != nil) {
        NSArray* _draggedNodes = [[[info draggingSource] dataSource] draggedNodes];
        //NSEnumerator* enumerator = [_draggedNodes objectEnumerator];
        //MyTree* node = nil;
        targetNodeIsValid = ![targetNode isDescendantOfNodeInArray: _draggedNodes];
        /*
         while ((node = [enumerator nextObject]) != nil)
            if ([[node parent] isEqualTo:SAFENODE(item)] && !isOnDropTypeProposal && childIndex >= [SAFENODE(item) numberOfChildren]) {
                targetNodeIsValid = NO;
                break;
            }
         */
    }

    
    // Set the item and child index in case we computed a retargeted one.
    [outlineView setDropItem:targetNode dropChildIndex:childIndex];
    
    return targetNodeIsValid ? NSDragOperationGeneric : NSDragOperationNone;
}

- (void)_performDropOperation:(id <NSDraggingInfo>)info onNode:(id)pnode atIndex:(int)childIndex {
    // Helper method to insert dropped data into the model. 
    NSPasteboard* pboard = [info draggingPasteboard];
    NSMutableArray* itemsToSelect = nil;
    MyTree* parentNode = SAFENODE(pnode);
    //NSLog(@"[FavoritesDrawerController _performDropOperation:%@ onNode:%@ atIndex:%d]",info,pnode,childIndex);
    //NSLog(@"parentNode = %@",parentNode);
    if ([pboard availableTypeFromArray:[NSArray arrayWithObjects:DragDropSimplePboardType, nil]] != nil) {
        FavoritesDrawerController *dragDataSource = [[info draggingSource] dataSource];
        NSArray *_draggedNodes = [MyTree minimumNodeCoverFromNodesInArray: [dragDataSource draggedNodes]];
        NSEnumerator *draggedNodesEnum = [_draggedNodes objectEnumerator];
        MyTree *_draggedNode = nil, *_draggedNodeParent = nil;
        //MyTree* favs = [CelestiaFavorites sharedFavorites];
	itemsToSelect = [NSMutableArray arrayWithArray:[self selectedNodes]];
        /*
        NSLog(@"dragDataSource = %@",dragDataSource);
        NSLog(@"_draggedNodes = %@",_draggedNodes);
        NSLog(@"itemsToSelect = %@",itemsToSelect);
         */
        while ((_draggedNode = [draggedNodesEnum nextObject])) {
            _draggedNodeParent = [_draggedNode parent];
            if (parentNode==_draggedNodeParent && [parentNode indexOfChild: _draggedNode]<childIndex) childIndex--;
            [_draggedNodeParent removeChild: _draggedNode];
        }
        [parentNode insertChildren: _draggedNodes atIndex: childIndex];
    } 

    [[CelestiaFavorites sharedFavorites] synchronize];
    [outlineView selectItems: itemsToSelect byExtendingSelection: NO];
}

- (BOOL)outlineView:(NSOutlineView*)olv acceptDrop:(id <NSDraggingInfo>)info item:(id)targetItem childIndex:(int)childIndex {
    MyTree * 		parentNode = nil;
    //NSLog(@"[FavoritesDrawerController outlineView:%@ acceptDrop:%@ item:%@ childIndex:%d]",olv,info,targetItem,childIndex);
    // Determine the parent to insert into and the child index to insert at.
    parentNode = (MyTree*)(targetItem ? targetItem : [CelestiaFavorites sharedFavorites]);//SAFENODE(targetItem);
    childIndex = (childIndex==NSOutlineViewDropOnItemIndex ? 0 : childIndex);
    [self _performDropOperation:info onNode:parentNode atIndex:childIndex];        
    return YES;
}

@end
