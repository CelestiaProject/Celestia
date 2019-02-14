//
//  CelestiaMath.h
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//


@interface CelestiaVector : NSArray /*<NSCoding>*/
{
    NSArray* _array;
}
-(void)encodeWithCoder:(NSCoder*)coder;
-(id)initWithCoder:(NSCoder*)coder;
+(CelestiaVector*)vectorWithArray:(NSArray*)v;
+(CelestiaVector*)vectorWithx:(NSNumber*)v y:(NSNumber*)y;
+(CelestiaVector*)vectorWithx:(NSNumber*)v y:(NSNumber*)y z:(NSNumber*)z;
+(CelestiaVector*)vectorWithx:(NSNumber*)v y:(NSNumber*)y z:(NSNumber*)z w:(NSNumber*)w;
-(CelestiaVector*)initWithArray:(NSArray*)v;
-(CelestiaVector*)initWithx:(NSNumber*)v y:(NSNumber*)y;
-(CelestiaVector*)initWithx:(NSNumber*)v y:(NSNumber*)y z:(NSNumber*)z;
-(CelestiaVector*)initWithx:(NSNumber*)v y:(NSNumber*)y z:(NSNumber*)z w:(NSNumber*)w;
-(unsigned)count;
-objectAtIndex:(unsigned)index;
-(NSNumber*)x;
-(NSNumber*)y;
-(NSNumber*)z;
-(NSNumber*)w;
@end
