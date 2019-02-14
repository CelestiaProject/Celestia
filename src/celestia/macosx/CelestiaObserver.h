//
//  CelestiaObserver.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaVector.h"
#import "CelestiaUniversalCoord.h"

@interface CelestiaObserver : NSObject {
    NSData* _data;
}

-(CelestiaUniversalCoord*)position;
/*
-(CelestiaVector*)orientation;
-(void)setOrientation:(CelestiaVector*)q;
-(CelestiaVector*)velocity;
-(void)setVelocity:(CelestiaVector*)v;
-(CelestiaVector*)angularVelocity;
-(void)setAngularVelocity:(CelestiaVector*)v;
 */
-(void)setPosition:(CelestiaUniversalCoord*)p;
-(void)setPositionWithPoint:(CelestiaVector*)p;
-(void)update:(NSNumber*)dt timeScale: (NSNumber*)ts;
-(unsigned int) getLocationFilter;
-(void) setLocationFilter: (unsigned int) filter;
@end
