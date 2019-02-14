//
//  CelestiaMath.mm
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaVector.h"
#import "CelestiaVector_PrivateAPI.h"

@implementation CelestiaVector(PrivateAPI)

/*** Conversions to and from Eigen structures ***/

+(CelestiaVector*)vectorWithVector3f:(const Eigen::Vector3f&)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x()] y:[NSNumber numberWithFloat:v.y()] z:[NSNumber numberWithFloat:v.z()]];
}

+(CelestiaVector*)vectorWithVector3d:(const Eigen::Vector3d&)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithDouble:v.x()] y:[NSNumber numberWithDouble:v.y()] z:[NSNumber numberWithDouble:v.z()]];
}

+(CelestiaVector*)vectorWithQuaternionf:(const Eigen::Quaternionf&)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x()] y:[NSNumber numberWithFloat:v.y()] z:[NSNumber numberWithFloat:v.z()] w:[NSNumber numberWithFloat:v.w()]];
}

+(CelestiaVector*)vectorWithQuaterniond:(const Eigen::Quaterniond&)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithDouble:v.x()] y:[NSNumber numberWithDouble:v.y()] z:[NSNumber numberWithDouble:v.z()] w:[NSNumber numberWithDouble:v.w()]];
}

-(CelestiaVector*)initWithVector3f:(const Eigen::Vector3f&)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x()] y:[NSNumber numberWithFloat:v.y()] z:[NSNumber numberWithFloat:v.z()]];
}

-(CelestiaVector*)initWithVector3d:(const Eigen::Vector3d&)v
{
    return [self initWithx:[NSNumber numberWithDouble:v.x()] y:[NSNumber numberWithDouble:v.y()] z:[NSNumber numberWithDouble:v.z()]];
}

-(Eigen::Vector3f)vector3f
{
    return Eigen::Vector3f([[self x] floatValue],[[self y] floatValue],[[self z] floatValue]);
}

-(Eigen::Vector3d)vector3d
{
    return Eigen::Vector3d([[self x] doubleValue],[[self y] doubleValue],[[self z] doubleValue]);
}

-(Eigen::Quaternionf)quaternionf
{
    return Eigen::Quaternionf([[self w] floatValue],[[self x] floatValue],[[self y] floatValue],[[self z] floatValue]);
}

-(Eigen::Quaterniond)quaterniond
{
    return Eigen::Quaterniond([[self w] doubleValue],[[self x] doubleValue],[[self y] doubleValue],[[self z] doubleValue]);
}

/*** End Eigen conversions ***/


+(CelestiaVector*)vectorWithVec2f:(Vec2f)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y]];
}
+(CelestiaVector*)vectorWithPoint2f:(Point2f)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y]];
}
+(CelestiaVector*)vectorWithVec3f:(Vec3f)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z]];
}
+(CelestiaVector*)vectorWithPoint3f:(Point3f)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z]];
}
+(CelestiaVector*)vectorWithVec4f:(Vec4f)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z] w:[NSNumber numberWithFloat:v.w]];
}
+(CelestiaVector*)vectorWithVec3d:(Vec3d)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z]];
}
+(CelestiaVector*)vectorWithPoint3d:(Point3d)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z]];
}
+(CelestiaVector*)vectorWithVec4d:(Vec4d)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z] w:[NSNumber numberWithDouble:v.w]];
}
+(CelestiaVector*)vectorWithQuatd:(Quatd)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z] w:[NSNumber numberWithDouble:v.w]];
}
+(CelestiaVector*)vectorWithQuatf:(Quatf)v
{
    return [CelestiaVector vectorWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z] w:[NSNumber numberWithFloat:v.w]];
}

-(CelestiaVector*)initWithVec2f:(Vec2f)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y]];
}
-(CelestiaVector*)initWithVec3f:(Vec3f)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z]];
}
-(CelestiaVector*)initWithVec4f:(Vec4f)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z] w:[NSNumber numberWithFloat:v.w]];
}
-(CelestiaVector*)initWithVec3d:(Vec3d)v
{
    return [self initWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z]];
}
-(CelestiaVector*)initWithVec4d:(Vec4d)v
{
    return [self initWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z] w:[NSNumber numberWithDouble:v.w]];
}
-(CelestiaVector*)initWithQuatd:(Quatd)v
{
    return [self initWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z] w:[NSNumber numberWithDouble:v.w]];
}
-(CelestiaVector*)initWithQuatf:(Quatf)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z] w:[NSNumber numberWithFloat:v.w]];
}

-(Vec2f)vec2f
{
    return Vec2f([[self x] floatValue],[[self y] floatValue]);
}
-(Vec3f)vec3f
{
    return Vec3f([[self x] floatValue],[[self y] floatValue],[[self z] floatValue]);
}
-(Point2f)point2f
{
    return Point2f([[self x] floatValue],[[self y] floatValue]);
}
-(Point3f)point3f
{
    return Point3f([[self x] floatValue],[[self y] floatValue],[[self z] floatValue]);
}
-(Vec4f)vec4f
{
    return Vec4f([[self x] floatValue],[[self y] floatValue],[[self z] floatValue], [[self w] floatValue]);
}
-(Vec3d)vec3d
{
    return Vec3d([[self x] doubleValue],[[self y] doubleValue],[[self z] doubleValue]);
}
-(Point3d)point3d
{
    return Point3d([[self x] doubleValue],[[self y] doubleValue],[[self z] doubleValue]);
}
-(Vec4d)vec4d
{
    return Vec4d([[self x] doubleValue],[[self y] doubleValue],[[self z] doubleValue], [[self w] doubleValue]);
}
-(Quatf)quatf
{
    return Quatf([[self w] floatValue],[[self x] floatValue],[[self y] floatValue],[[self z] floatValue]);
}
-(Quatd)quatd
{
    return Quatd([[self w] doubleValue],[[self x] doubleValue],[[self y] doubleValue],[[self z] doubleValue]);
}
-(CelestiaVector*)initWithPoint2f:(Point2f)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y]];
}
-(CelestiaVector*)initWithPoint3d:(Point3d)v
{
    return [self initWithx:[NSNumber numberWithDouble:v.x] y:[NSNumber numberWithDouble:v.y] z:[NSNumber numberWithDouble:v.z]];
}
-(CelestiaVector*)initWithPoint3f:(Point3f)v
{
    return [self initWithx:[NSNumber numberWithFloat:v.x] y:[NSNumber numberWithFloat:v.y] z:[NSNumber numberWithFloat:v.z]];
}
@end

@implementation CelestiaVector
-(void)encodeWithCoder:(NSCoder*)coder
{
    //[super encodeWithCoder:coder];
    [coder encodeObject:_array];
}
-(id)initWithCoder:(NSCoder*)coder
{
    //self = [super initWithCoder:coder];
    self = [self init];
    _array = [[coder decodeObject] retain];
    return self;
}
-(void)dealloc
{
    if (_array != nil) {
        [_array release];
        _array = nil;
    }
    [super dealloc];
}
+(CelestiaVector*)vectorWithArray:(NSArray*)v
{
    return [[[CelestiaVector alloc] initWithArray:v] autorelease];
}
+(CelestiaVector*)vectorWithx:(NSNumber*)x y:(NSNumber*)y
{
    return [[[CelestiaVector alloc] initWithx:x y:y] autorelease];
}
+(CelestiaVector*)vectorWithx:(NSNumber*)x y:(NSNumber*)y z:(NSNumber*)z
{
    return [[[CelestiaVector alloc] initWithx:x y:y z:z] autorelease];
}
+(CelestiaVector*)vectorWithx:(NSNumber*)x y:(NSNumber*)y z:(NSNumber*)z w:(NSNumber*)w
{
    return [[[CelestiaVector alloc] initWithx:x y:y z:z w:w] autorelease];
}
-(CelestiaVector*)initWithArray:(NSArray*)v
{
    self = [super init];
    _array = [v retain];
    return self;
}
-(CelestiaVector*)initWithx:(NSNumber*)x y:(NSNumber*)y
{
    self = [super init];
    _array = [[NSArray alloc] initWithObjects:x,y,nil];
    return self;
}
-(CelestiaVector*)initWithx:(NSNumber*)x y:(NSNumber*)y z:(NSNumber*)z
{
    self = [super init];
    _array = [[NSArray alloc] initWithObjects:x,y,z,nil];
    return self;
}
-(CelestiaVector*)initWithx:(NSNumber*)x y:(NSNumber*)y z:(NSNumber*)z w:(NSNumber*)w
{
    self = [super init];
    _array = [[NSArray alloc] initWithObjects:x,y,z,w,nil];
    return self;
}
-(NSNumber*)x
{
    return [_array objectAtIndex:0];
}
-(NSNumber*)y
{
    return [_array objectAtIndex:1];
}
-(NSNumber*)z
{
    return [_array objectAtIndex:2];
}
-(NSNumber*)w
{
    return [_array objectAtIndex:3];
}
-objectAtIndex:(unsigned)index
{
    return [_array objectAtIndex:index];
}
-(unsigned)count
{
    return [_array count];
}
@end