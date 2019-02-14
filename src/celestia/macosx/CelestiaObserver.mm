//
//  CelestiaObserver.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaObserver.h"
#import "CelestiaObserver_PrivateAPI.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"

@implementation CelestiaObserver(PrivateAPI)
-(Observer)observer
{
    return *reinterpret_cast<const Observer*>([_data bytes]);
}
-(CelestiaObserver*)initWithObserver:(Observer)obs
{
    self=[super init];
    _data = [[NSData alloc] initWithBytes:(void *)&obs length:sizeof(Observer)];
    return self;
}
@end

@implementation CelestiaObserver
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(CelestiaUniversalCoord*)position
{
    return [[[CelestiaUniversalCoord alloc] initWithUniversalCoord:[self observer].getPosition()] autorelease];
}
/*
-(CelestiaVector*)orientation
{
    return [CelestiaVector vectorWithQuatf:[self observer].getOrientationf()];
}
-(void)setOrientation:(CelestiaVector*)q
{
    [self observer].setOrientation([q quatd]);
}
-(CelestiaVector*)velocity
{
    return [CelestiaVector vectorWithVec3d:[self observer].getVelocity()];
}
-(void)setVelocity:(CelestiaVector*)v
{
    [self observer].setVelocity([v vec3d]);
}
-(CelestiaVector*)angularVelocity
{
    return [CelestiaVector vectorWithVec3d:[self observer].getAngularVelocity()];
}
-(void)setAngularVelocity:(CelestiaVector*)v
{
    [self observer].setAngularVelocity([v vec3d]);
}
 */
-(void)setPosition:(CelestiaUniversalCoord*)p
{
    [self observer].setPosition([p universalCoord]);
}
-(void)setPositionWithPoint:(CelestiaVector*)p
{
    [self observer].setPosition([p point3d]);
}
-(void)update:(NSNumber*)dt timeScale: (NSNumber*)ts
{
    [self observer].update([dt doubleValue], [ts doubleValue]);
}

-(unsigned int) getLocationFilter
{
    return [self observer].getLocationFilter();
}

-(void) setLocationFilter: (unsigned int) filter
{
    [self observer].setLocationFilter(filter);
}

@end