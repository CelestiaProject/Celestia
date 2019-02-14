//
//  CelestiaStar.mm
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaStar.h"
#import "CelestiaStar_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"
#import "CelestiaAppCore.h"
#import "CelestiaAppCore_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"

@implementation CelestiaStar(PrivateAPI)
-(CelestiaStar*)initWithStar:(const Star*)s
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&s) objCType:@encode(Star*)];
    return self;
}
-(Star*)star
{
    return reinterpret_cast<Star*>([_data pointerValue]);
}
@end
/* 
    inline StellarClass getStellarClass() const;
    void setStellarClass(StellarClass);
*/
@implementation CelestiaStar
-(void)setCatalogNumber:(NSNumber*)cat
{
    [self star]->setCatalogNumber([cat unsignedIntValue]);
}
-(void)setCatalogNumber:(NSNumber*)cat atIndex:(NSNumber*)index
{
//    [self star]->setCatalogNumber([index unsignedIntValue], [cat unsignedIntValue]);
}
-(NSNumber*)catalogNumber
{
    return [NSNumber numberWithUnsignedInt:[self star]->getCatalogNumber()];
}
-(NSNumber*)catalogNumberAtIndex:(NSNumber*)cat
{
//    return [NSNumber numberWithUnsignedInt:[self star]->getCatalogNumber([cat unsignedIntValue])];
}
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}

-(NSNumber *)radius
{
    return [NSNumber numberWithFloat:[self star]->getRadius()];
}
/*
-(CelestiaVector*)position
{
    return [CelestiaVector vectorWithPoint3f:[self star]->getPosition()];
}
-(void)setPosition:(CelestiaVector*)p
{
    [self star]->setPosition([p point3f]);
}
 */
-(NSNumber*)apparentMagnitude:(NSNumber*)m
{
    return [NSNumber numberWithFloat:[self star]->getApparentMagnitude([m floatValue])];
}
-(NSNumber*)luminosity
{
    return [NSNumber numberWithFloat:[self star]->getLuminosity()];
}
-(NSNumber*)temperature
{
    return [NSNumber numberWithFloat:[self star]->getTemperature()];
}
-(NSNumber*)rotationPeriod
{
//    return [NSNumber numberWithFloat:[self star]->getRotationPeriod()];
}
-(void)setAbsoluteMagnitude:(NSNumber*)m
{
    [self star]->setAbsoluteMagnitude([m floatValue]);
}
-(void)setLuminosity:(NSNumber*)m
{
    [self star]->setLuminosity([m floatValue]);
}

-(NSString *)name
{
    return [NSString stringWithStdString:[[CelestiaAppCore sharedAppCore] appCore]->getSimulation()->getUniverse()->getStarCatalog()->getStarName(*[self star])];
}
@end
