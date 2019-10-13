//
//  CelestiaStar.h
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaVector.h"

@interface CelestiaStar : NSObject {
    NSValue* _data;
}
-(NSNumber*)catalogNumber;
-(void)setCatalogNumber:(NSNumber*)cat;
-(void)setCatalogNumber:(NSNumber*)cat atIndex:(NSNumber*)index;
-(NSNumber *)radius;
-(CelestiaVector*)position;
-(void)setPosition:(CelestiaVector*)p;
-(NSNumber*)apparentMagnitude:(NSNumber*)m;
-(NSNumber*)luminosity;
-(NSNumber*)temperature;
-(void)setAbsoluteMagnitude:(NSNumber*)m;
-(void)setLuminosity:(NSNumber*)m;
-(NSString *)name;
@end
