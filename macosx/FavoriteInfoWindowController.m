#import "FavoriteInfoWindowController.h"

@implementation FavoriteInfoWindowController
-(void)awakeFromNib
{
    _fav = nil;
}
- (IBAction)showWindow:(id)sender
{
    NSLog(@"[FavoriteInfoWindowController showWindow:%@]",sender);
    [super showWindow:sender];
}
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
    [jdField setDoubleValue:[[fav jd] doubleValue]];
    [coordinateField setStringValue:[fav coordinateSystem]];
    [orientationField setStringValue:[[fav orientation] description]];
}
- (IBAction)navigateTo:(id)sender
{
    if (_fav == nil)
        return;
    [[CelestiaAppCore sharedAppCore] activateFavorite:_fav];
}

@end
