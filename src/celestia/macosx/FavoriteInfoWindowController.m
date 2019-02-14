#import "FavoriteInfoWindowController.h"

@implementation FavoriteInfoWindowController

-(void)updateFavorite:(CelestiaFavorite*)fav
{
    _fav = fav;
    if (_fav == nil) {
        [[self window] performClose:self];
        return;
    }
    if ([fav isFolder]) {
        [[self window] performClose:self];
        _fav = nil;
        return;
    }
    [nameField setStringValue:[fav name]];
    [selectionField setStringValue:[fav selectionName]];
    [jdField setStringValue:[[NSDate dateWithJulian:[fav jd]] description]];
    [coordinateField setStringValue:[fav coordinateSystem]];
    [orientationField setStringValue:[[fav orientation] description]];
}
- (IBAction)navigateTo:(id)sender
{
    [[self window] performClose:self];
    if (_fav != nil)
        [(FavoritesDrawerController*)favoritesDrawerController activateFavorite:_fav];
}

@end
