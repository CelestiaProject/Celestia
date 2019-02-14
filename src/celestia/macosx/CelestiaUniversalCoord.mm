//
//  CelestiaUniversalCoord.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaUniversalCoord.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"

@implementation CelestiaUniversalCoord(PrivateAPI)
-(UniversalCoord)universalCoord
{
    return *reinterpret_cast<const UniversalCoord*>([_data bytes]);
}
-(CelestiaUniversalCoord*)initWithUniversalCoord:(UniversalCoord)uni
{
    self = [super init];
    _data = [[NSData alloc] initWithBytes:(void *)&uni length:sizeof(UniversalCoord)];
    return self;
}
@end

@implementation CelestiaUniversalCoord
-(NSData*)data
{
    return _data;
}
-(id)initWithData:(NSData*)data
{
    self = [super init];
    _data = [data retain];
    return self;
}
-(void)encodeWithCoder:(NSCoder*)coder
{
    NSLog(@"[CelestiaUniversalCoord encodeWithCoder:%@]",coder);
    //[super encodeWithCoder:coder];
    [coder encodeObject:_data];
}
-(id)initWithCoder:(NSCoder*)coder
{
    NSLog(@"[CelestiaUniversalCoord initWithCoder:%@]",coder);
    //self = [super initWithCoder:coder];
    self = [super init];
    _data = [[coder decodeObject] retain];
    return self;
}
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(CelestiaVector*)celestiaVector
{
    return [CelestiaVector vectorWithPoint3d:Point3d([self universalCoord])];
}
-(NSNumber*)distanceTo:(CelestiaUniversalCoord*)t
{
    return [NSNumber numberWithDouble:[self universalCoord].distanceTo([t universalCoord])];
}
-(CelestiaUniversalCoord*)difference:(CelestiaUniversalCoord*)t
{
    return [[[CelestiaUniversalCoord alloc] initWithUniversalCoord:[self universalCoord].difference([self universalCoord])] autorelease];
}
@end