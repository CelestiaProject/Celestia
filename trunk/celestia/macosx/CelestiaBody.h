//
//  CelestiaBody.h
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#import "CelestiaVector.h"

@interface CelestiaBody : NSObject {
    NSValue* _data;
}
-(NSString*)classification;
-(NSString*)name;
-(NSNumber*)radius;
-(NSNumber*)mass;
-(NSNumber*)oblateness;
-(NSNumber*)albedo;
-(CelestiaVector*)orientation;
-(void)setOrientation:(CelestiaVector*)q;
-(void)setName:(NSString*)s;
-(void)setRadius:(NSNumber*)r;
-(void)setMass:(NSNumber*)m;
-(void)setOblateness:(NSNumber*)o;
-(void)setAlbedo:(NSNumber*)a;
-(CelestiaVector*)heliocentricPosition:(NSNumber*)n;
-(CelestiaVector*)equatorialToBodyFixed:(NSNumber*)n;
-(CelestiaVector*)eclipticalToEquatorial:(NSNumber*)n;
-(CelestiaVector*)eclipticalToBodyFixed:(NSNumber*)n;
-(NSArray*)alternateSurfaceNames;
@end
