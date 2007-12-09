//
//  CelestiaBody.mm
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#import "CelestiaBody.h"
#import "CelestiaBody_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"

@implementation CelestiaBody(PrivateAPI)
-(CelestiaBody*)initWithBody:(Body*)b
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&b) objCType:@encode(Body*)];
    return self;
}
-(Body*)body
{
    return reinterpret_cast<Body*>([_data pointerValue]);
}
@end


@implementation CelestiaBody
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}

-(NSString*)classification
{
    switch ([self body]->getClassification())
    {
        case (Body::Planet):
            return NSLocalizedString(@"Planet",@"");
            break;
        case (Body::Moon):
            return NSLocalizedString(@"Moon",@"");
            break;
        case (Body::Asteroid):
            return NSLocalizedString(@"Asteroid",@"");
            break;
        case (Body::Comet):
            return NSLocalizedString(@"Comet",@"");
            break;
        case (Body::Spacecraft):
            return NSLocalizedString(@"Spacecraft",@"");
            break;
        default:
            break;
    }
    return NSLocalizedString(@"Unknown",@"");
}
-(NSString *)name
{
    return [NSString stringWithStdString:[self body]->getName(true)];
}
-(NSNumber *)radius
{
    return [NSNumber numberWithFloat:[self body]->getRadius()];
}
-(NSNumber *)mass
{
    return [NSNumber numberWithFloat:[self body]->getMass()];
}
-(NSNumber *)oblateness
{
    return [NSNumber numberWithFloat:[self body]->getOblateness()];
}
-(NSNumber *)albedo
{
    return [NSNumber numberWithFloat:[self body]->getAlbedo()];
}
-(CelestiaVector*)orientation
{
    return [CelestiaVector vectorWithQuatf:[self body]->getOrientation()];
}
-(void)setOrientation:(CelestiaVector*)q
{
    [self body]->setOrientation([q quatf]);
}
-(void)setName:(NSString*)s
{
    [self body]->setName([s stdString]);
}
-(void)setRadius:(NSNumber*)r
{
    [self body]->setRadius([r floatValue]);
}
-(void)setMass:(NSNumber*)m
{
    [self body]->setMass([m floatValue]);
}
-(void)setOblateness:(NSNumber*)o
{
    [self body]->setOblateness([o floatValue]);
}
-(void)setAlbedo:(NSNumber*)a
{
    [self body]->setAlbedo([a floatValue]);
}
-(CelestiaVector*)heliocentricPosition:(NSNumber*)n
{
    return [CelestiaVector vectorWithPoint3d:[self body]->getHeliocentricPosition([n doubleValue])];
}
-(CelestiaVector*)equatorialToBodyFixed:(NSNumber*)n
{
    return [CelestiaVector vectorWithQuatd:[self body]->getEquatorialToBodyFixed([n doubleValue])];
}
-(CelestiaVector*)eclipticalToEquatorial:(NSNumber*)n
{
    return [CelestiaVector vectorWithQuatd:[self body]->getEclipticalToEquatorial([n doubleValue])];
}
-(CelestiaVector*)eclipticalToBodyFixed:(NSNumber*)n
{
    return [CelestiaVector vectorWithQuatd:[self body]->getEclipticalToBodyFixed([n doubleValue])];
}

-(NSArray*)alternateSurfaceNames
{
    NSMutableArray *result = nil;
    std::vector<std::string>* altSurfaces = [self body]->getAlternateSurfaceNames();
    if (altSurfaces)
    {
        result = [NSMutableArray array];
        if (altSurfaces->size() > 0)
        {
            for (unsigned int i = 0; i < altSurfaces->size(); ++i)
            {
                [result addObject: [NSString stringWithStdString: (*altSurfaces)[i].c_str()]];
            }
        }
        delete altSurfaces;
    }
    return result;
}
@end
