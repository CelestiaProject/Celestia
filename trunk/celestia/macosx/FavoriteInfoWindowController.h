/* FavoriteInfoWindowController */

#import <Cocoa/Cocoa.h>
#import "CelestiaFavorite.h"
#import "CelestiaAppCore.h"
@interface FavoriteInfoWindowController : NSWindowController
{
    IBOutlet NSTextField *coordinateField;
    IBOutlet NSTextField *jdField;
    IBOutlet NSTextField *nameField;
    IBOutlet NSTextField *orientationField;
    IBOutlet NSTextField *selectionField;
    CelestiaFavorite* _fav;
}
-(IBAction)showWindow:(id)sender;
-(IBAction)navigateTo:(id)sender;
-(void)updateFavorite:(CelestiaFavorite*)fav;
@end
