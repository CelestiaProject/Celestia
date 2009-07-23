//
//  CelestiaGalaxy.mm
//  celestia
//
//  Created by Bob Ippolito on Sat Jun 08 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaGalaxy.h"
#import "CelestiaGalaxy_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaAppCore.h"
#include "celestiacore.h"


@implementation CelestiaGalaxy(PrivateAPI)
-(CelestiaGalaxy*)initWithGalaxy:(Galaxy*)g
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&g) objCType:@encode(Galaxy*)];
    return self;
}
-(Galaxy*)galaxy
{
    return reinterpret_cast<Galaxy*>([_data pointerValue]);
}
@end

/*
    GalacticForm* getForm() const;
*/

@implementation CelestiaGalaxy

-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(NSString*)type
{
    return [NSString stringWithUTF8String:[self galaxy]->getType()];
}

//-(void)setType:(NSString*)s
//{
//    [self galaxy]->setType([s stdString]);
//}

-(NSString *)name
{
    return [NSString stringWithStdString:[[CelestiaAppCore sharedAppCore] appCore]->getSimulation()->getUniverse()->getDSOCatalog()->getDSOName([self galaxy])];
}
/*
-(CelestiaVector*)orientation
{
    return [CelestiaVector vectorWithQuatf:[self galaxy]->getOrientation()];
}
-(void)setOrientation:(CelestiaVector*)q
{
    [self galaxy]->setOrientation([q quatf]);
}
-(CelestiaVector*)position
{
    return [CelestiaVector vectorWithPoint3d:[self galaxy]->getPosition()];
}
-(void)setPosition:(CelestiaVector*)q
{
    [self galaxy]->setPosition([q point3d]);
}
 */
//-(void)setName:(NSString*)s
//{
//    [self galaxy]->setName([s stdString]);
//}
-(NSNumber *)radius
{
    return [NSNumber numberWithFloat:[self galaxy]->getRadius()];
}
-(void)setRadius:(NSNumber*)r
{
    [self galaxy]->setRadius([r floatValue]);
}
-(NSNumber *)detail
{
    return [NSNumber numberWithFloat:[self galaxy]->getDetail()];
}
-(void)setDetail:(NSNumber*)d
{
    [self galaxy]->setDetail([d floatValue]);
}
@end
