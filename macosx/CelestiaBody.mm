//
//  CelestiaBody.mm
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
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
            return @"Planet";
            break;
        case (Body::Moon):
            return @"Moon";
            break;
        case (Body::Asteroid):
            return @"Asteroid";
            break;
        case (Body::Comet):
            return @"Comet";
            break;
        case (Body::Spacecraft):
            return @"Spacecraft";
            break;
        default:
            break;
    }
    return @"Unknown";
}
-(NSString *)name
{
    return [NSString stringWithStdString:[self body]->getName()];
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
-(CelestiaVector*)equatorialToGeographic:(NSNumber*)n
{
    return [CelestiaVector vectorWithQuatd:[self body]->getEquatorialToGeographic([n doubleValue])];
}
-(CelestiaVector*)eclipticalToEquatorial:(NSNumber*)n
{
    return [CelestiaVector vectorWithQuatd:[self body]->getEclipticalToEquatorial([n doubleValue])];
}
-(CelestiaVector*)eclipticalToGeographic:(NSNumber*)n
{
    return [CelestiaVector vectorWithQuatd:[self body]->getEclipticalToGeographic([n doubleValue])];
}
/*
    Body(PlanetarySystem*);

    PlanetarySystem* getSystem() const;
    Orbit* getOrbit() const;
    void setOrbit(Orbit*);
    RotationElements getRotationElements() const;
    void setRotationElements(const RotationElements&);
    void setClassification(int);

    PlanetarySystem* getSatellites() const;
    void setSatellites(PlanetarySystem*);

    RingSystem* getRings() const;
    void setRings(const RingSystem&);
    const Atmosphere* getAtmosphere() const;
    Atmosphere* getAtmosphere();
    void setAtmosphere(const Atmosphere&);

    void setMesh(ResourceHandle);
    ResourceHandle getMesh() const;
    void setSurface(const Surface&);
    const Surface& getSurface() const;
    Surface& getSurface();

    float getLuminosity(const Star& sun,
                        float distanceFromSun) const;
    float getApparentMagnitude(const Star& sun,
                               float distanceFromSun,
                               float distanceFromViewer) const;
    float getApparentMagnitude(const Star& sun,
                               const Vec3d& sunPosition,
                               const Vec3d& viewerPosition) const;

    Mat4d getLocalToHeliocentric(double) const;
    Mat4d getGeographicToHeliocentric(double);
*/
@end
