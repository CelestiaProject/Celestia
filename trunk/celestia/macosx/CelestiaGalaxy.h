//
//  CelestiaGalaxy.h
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaVector.h"

@interface CelestiaGalaxy : NSObject {
    NSValue* _data;
}
-(NSString*)type;
//-(void)setType:(NSString*)s;
-(NSString *)name;
/*
-(CelestiaVector*)orientation;
-(void)setOrientation:(CelestiaVector*)q;
-(CelestiaVector*)position;
-(void)setPosition:(CelestiaVector*)q;
 */
//-(void)setName:(NSString*)s;
-(NSNumber *)radius;
-(void)setRadius:(NSNumber*)r;
-(NSNumber *)detail;
-(void)setDetail:(NSNumber*)d;
@end
