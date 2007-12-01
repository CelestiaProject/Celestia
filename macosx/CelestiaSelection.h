//
//  CelestiaSelection.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaStar.h"
#import "CelestiaBody.h"
#import "CelestiaGalaxy.h"
#import "CelestiaUniversalCoord.h"

@interface CelestiaSelection : NSObject {
    NSMutableData *_data;
}
-(CelestiaSelection*)initWithCelestiaStar:(CelestiaStar*)s;
-(CelestiaSelection*)initWithCelestiaBody:(CelestiaBody*)b;
-(CelestiaSelection*)initWithCelestiaGalaxy:(CelestiaGalaxy*)g;
-(BOOL)isEmpty;
-(NSNumber*)radius;
-(BOOL)isEqualToSelection:(CelestiaSelection*)csel;
-(CelestiaStar*)star;
-(CelestiaBody*)body;
-(CelestiaGalaxy*)galaxy;
-(NSString *)name;
-(NSString *)briefName;
-(CelestiaUniversalCoord*)position:(NSNumber*)t;
@end
