/* FavoritesDrawerController */

#import "CelestiaFavorite.h";
#import "ContextOutlineView.h";
#import "FavoriteInfoWindowController.h"
#import "NSWindowController_Extensions.h"


@interface FavoritesDrawerController : NSWindowController
{
    NSArray *draggedNodes;
    IBOutlet ContextOutlineView *outlineView;
    IBOutlet NSMenu *favoritesMenu;
    IBOutlet NSMenu *favoritesContextMenu;
    IBOutlet id favoriteInfoWindowController;
}
-(NSArray*)selectedNodes;
-(NSArray*)draggedNodes;
-(void)activateFavorite:(CelestiaFavorite*)fav;
-(NSMenu *)outlineView:(NSOutlineView *)outlineView 
contextMenuForItem:(id)item;
-(IBAction)addNewFavorite:(id)sender;
-(IBAction)addNewFolder:(id)sender;
-(IBAction)doubleClick:(id)sender;
-(void)synchronizeFavoritesMenu;
-(id)outlineView:(NSOutlineView*)olv child:(int)index ofItem:(id)item;
-(BOOL)outlineView:(NSOutlineView*)olv isItemExpandable:(id)item;
-(int)outlineView:(NSOutlineView *)olv numberOfChildrenOfItem:(id)item;
-(id)outlineView:(NSOutlineView *)olv objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
-(void)outlineView:(NSOutlineView *)olv setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
-(void)outlineView:(NSOutlineView*)olv deleteItem:(id)item;
-(void)outlineView:(NSOutlineView*)olv editItem:(id)item;
-(void)outlineViewSelectionDidChange:(NSNotification*)notification;
-(BOOL)outlineView:(NSOutlineView *)olv writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard;
-(unsigned int)outlineView:(NSOutlineView*)olv validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(int)childIndex;
-(void)_performDropOperation:(id <NSDraggingInfo>)info onNode:(id)pnode atIndex:(int)childIndex;
-(BOOL)outlineView:(NSOutlineView*)olv acceptDrop:(id <NSDraggingInfo>)info item:(id)targetItem childIndex:(int)childIndex;
@end

@interface CelestiaFavorite(ViewAPI)
-(NSMenuItem*)favoriteMenuItem;
-(NSMenuItem*)setupFavoriteMenuItem:(NSMenuItem*)menuItem;
@end

@interface CelestiaFavorites(ViewAPI)
-(NSArray*)favoriteMenuItems;
@end

@interface MyTree(ViewAPI)
-(void)activate;
-(NSMenuItem*)favoriteMenuItem;
@end
