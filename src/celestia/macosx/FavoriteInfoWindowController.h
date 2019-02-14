/* FavoriteInfoWindowController */

#import "CelestiaFavorite.h"
#import "CelestiaAppCore.h"
#import "FavoritesDrawerController.h"
// NSDate dateWithJulian
#import "Astro.h"

@interface FavoriteInfoWindowController : NSWindowController
{
    IBOutlet NSTextField *coordinateField;
    IBOutlet NSTextField *jdField;
    IBOutlet NSTextField *nameField;
    IBOutlet NSTextField *orientationField;
    IBOutlet NSTextField *selectionField;
    IBOutlet id favoritesDrawerController;
    CelestiaFavorite* _fav;
}
-(IBAction)navigateTo:(id)sender;
-(void)updateFavorite:(CelestiaFavorite*)fav;
@end
