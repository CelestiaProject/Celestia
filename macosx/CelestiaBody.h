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
-(NSNumber*)albedo;
/*
-(CelestiaVector*)geometryOrientation;
-(void)setGeometryOrientation:(CelestiaVector*)q;
 */
-(void)setName:(NSString*)s;
-(void)setMass:(NSNumber*)m;
-(void)setAlbedo:(NSNumber*)a;
/*
-(CelestiaVector*)astrocentricPosition:(NSNumber*)n;
-(CelestiaVector*)equatorialToBodyFixed:(NSNumber*)n;
-(CelestiaVector*)eclipticToEquatorial:(NSNumber*)n;
-(CelestiaVector*)eclipticToBodyFixed:(NSNumber*)n;
 */
-(NSArray*)alternateSurfaceNames;
@end
