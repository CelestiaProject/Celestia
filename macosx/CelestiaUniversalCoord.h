//
//  CelestiaUniversalCoord.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaVector.h"
#import "CelestiaUniversalCoord.h"

@interface CelestiaUniversalCoord : NSObject /*<NSCoding>*/ {
    NSData* _data;
}
-(void)encodeWithCoder:(NSCoder*)coder;
-(id)initWithCoder:(NSCoder*)coder;
-(NSData*)data;
-(id)initWithData:(NSData*)data;
-(CelestiaVector*)celestiaVector;
-(NSNumber*)distanceTo:(CelestiaUniversalCoord*)t;
-(CelestiaUniversalCoord*)difference:(CelestiaUniversalCoord*)t;
@end
