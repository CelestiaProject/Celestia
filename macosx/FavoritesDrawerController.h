/* FavoritesDrawerController */

#import <Cocoa/Cocoa.h>
#import "CelestiaFavorite.h";
#import "ContextOutlineView.h";
#import "FavoriteInfoWindowController.h"


@interface FavoritesDrawerController : NSObject
{
    IBOutlet NSDrawer *drawer;
    IBOutlet ContextOutlineView *outlineView;
    IBOutlet NSMenu *favoritesMenu;
    IBOutlet NSMenu *favoritesContextMenu;
    IBOutlet id favoriteInfoWindowController;
}
-(void)activateFavorite:(CelestiaFavorite*)fav;
-(void)close;
-(IBAction)close:(id)sender;
- (NSMenu *)outlineView:(NSOutlineView *)outlineView 
contextMenuForItem:(id)item;
-(IBAction)addNewFavorite:(id)sender;
-(IBAction)addNewFolder:(id)sender;
-(IBAction)doubleClick:(id)sender;
-(void)synchronizeFavoritesMenu:(CelestiaFavorites*)favs;
-(id)outlineView:(NSOutlineView*)olv child:(int)index ofItem:(id)item;
-(BOOL)outlineView:(NSOutlineView*)olv isItemExpandable:(id)item;
-(int)outlineView:(NSOutlineView *)olv numberOfChildrenOfItem:(id)item;
-(id)outlineView:(NSOutlineView *)olv objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
-(void)outlineView:(NSOutlineView *)olv setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
-(void)outlineView:(NSOutlineView*)olv deleteItem:(CelestiaFavorite*)item;
-(void)outlineView:(NSOutlineView*)olv editItem:(id)item;
-(void)outlineViewSelectionDidChange:(NSNotification*)notification;
/*
-(BOOL)outlineView:(NSOutlineView *)olv shouldExpandItem:(id)item;
-(void)outlineView:(NSOutlineView *)olv willDisplayCell:(NSCell *)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item;
-(BOOL)outlineView:(NSOutlineView *)olv writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard;
-(unsigned int)outlineView:(NSOutlineView*)olv validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(int)childIndex;
-(void)_performDropOperation:(id <NSDraggingInfo>)info onNode:(TreeNode*)parentNode atIndex:(int)childIndex;
-(BOOL)outlineView:(NSOutlineView*)olv acceptDrop:(id <NSDraggingInfo>)info item:(id)targetItem childIndex:(int)childIndex;
*/

@end

@interface CelestiaFavorite(ViewAPI)
-(NSMenuItem*)menuItem;
-(NSMenuItem*)setupMenuItem:(NSMenuItem*)menuItem;
@end

@interface CelestiaFavorites(ViewAPI)
-(void)synchronizeMenu:(NSMenu*)menu atIndex:(unsigned)count;
@end
