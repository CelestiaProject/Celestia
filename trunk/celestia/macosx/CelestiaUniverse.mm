//
//  CelestiaUniverse.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaUniverse.h"
#import "CelestiaUniverse_PrivateAPI.h"
/*
#import "CelestiaVector_PrivateAPI.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaSelection_PrivateAPI.h"
*/


@implementation CelestiaUniverse(PrivateAPI)
-(CelestiaUniverse*)initWithUniverse:(Universe*)uni
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&uni) objCType:@encode(Universe*)];
    return self;
}
-(Universe*)universe
{
    return reinterpret_cast<Universe*>([_data pointerValue]);
}
@end

@implementation CelestiaUniverse
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
/*
-(CelestiaSelection*)pick:(CelestiaUniversalCoord*)origin direction:(CelestiaVector*)direction when:(NSNumber*)when faintestMag:(NSNumber*)faintestMag tolerance:(NSNumber*)tolerance
{
    return [[[CelestiaSelection alloc] initWithSelection:[self universe]->pick([origin universalCoord],[direction vec3f],[when floatValue],[faintestMag floatValue],[tolerance floatValue])] autorelease];
}
-(CelestiaSelection*)pick:(CelestiaUniversalCoord*)origin direction:(CelestiaVector*)direction when:(NSNumber*)when faintestMag:(NSNumber*)faintestMag
{
    return [[[CelestiaSelection alloc] initWithSelection:[self universe]->pick([origin universalCoord],[direction vec3f],[when floatValue],[faintestMag floatValue])] autorelease];
}
-(CelestiaSelection*)pickStar:(CelestiaUniversalCoord*)origin direction:(CelestiaVector*)direction faintestMag:(NSNumber*)faintestMag tolerance:(NSNumber*)tolerance
{
    return [[[CelestiaSelection alloc] initWithSelection:[self universe]->pickStar([origin universalCoord],[direction vec3f],[faintestMag floatValue],[tolerance floatValue])] autorelease];
}
*/

@end
