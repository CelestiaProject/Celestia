/*
 *  CelestiaFavorite_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celestia/favorites.h>

@interface CelestiaFavorite(PrivateAPI)
-(CelestiaFavorite*)initWithFavorite:(FavoritesEntry*)fav;
-(FavoritesEntry*)favorite;
-(void)setSelectionName:(NSString*)name;
-(void)setPosition:(CelestiaUniversalCoord*)position;
-(void)setOrientation:(CelestiaVector*)orientation;
-(void)setJd:(NSNumber*)jd;
-(void)setCoordinateSystem:(NSString*)coordSys;
@end