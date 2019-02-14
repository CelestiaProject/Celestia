//
//  CelestiaUniverse.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//
/*
#import "CelestiaSelection.h"
#import "CelestiaUniversalCoord.h"
*/
@interface CelestiaUniverse : NSObject {
    NSValue* _data;
}
/*
-(CelestiaSelection*)pick:(CelestiaUniversalCoord*)origin direction:(CelestiaVector*)direction when:(NSNumber*)when faintestMag:(NSNumber*)faintestMag tolerance:(NSNumber*)tolerance;
-(CelestiaSelection*)pick:(CelestiaUniversalCoord*)origin direction:(CelestiaVector*)direction when:(NSNumber*)when faintestMag:(NSNumber*)faintestMag;
-(CelestiaSelection*)pickStar:(CelestiaUniversalCoord*)origin direction:(CelestiaVector*)direction faintestMag:(NSNumber*)faintestMag tolerance:(NSNumber*)tolerance;
*/
@end
