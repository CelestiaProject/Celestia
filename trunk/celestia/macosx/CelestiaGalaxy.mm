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

NSDictionary* galaxyTypeDict;
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
+(void)initialize
{
    galaxyTypeDict = [[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:Galaxy::S0],@"S0",[NSNumber numberWithInt:Galaxy::Sa],@"Sa",[NSNumber numberWithInt:Galaxy::Sb],@"Sb",[NSNumber numberWithInt:Galaxy::Sc],@"Sc",[NSNumber numberWithInt:Galaxy::SBa],@"SBa",[NSNumber numberWithInt:Galaxy::SBb],@"SBb",[NSNumber numberWithInt:Galaxy::SBc],@"SBc",[NSNumber numberWithInt:Galaxy::E0],@"E0",[NSNumber numberWithInt:Galaxy::E1],@"E1",[NSNumber numberWithInt:Galaxy::E2],@"E2",[NSNumber numberWithInt:Galaxy::E2],@"E3",[NSNumber numberWithInt:Galaxy::E2],@"E4",[NSNumber numberWithInt:Galaxy::E2],@"E5",[NSNumber numberWithInt:Galaxy::E2],@"E6",[NSNumber numberWithInt:Galaxy::E2],@"E7",[NSNumber numberWithInt:Galaxy::Irr],@"Irr",nil,nil] retain];
}
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
    NSArray* keys = [galaxyTypeDict allKeys];
    NSNumber* n = [NSNumber numberWithInt:[self galaxy]->getType()];
    unsigned int i;
    for (i=0;i<[keys count];i++) {
        if ([[galaxyTypeDict objectForKey:[keys objectAtIndex:i]] isEqualToNumber:n])
            return [keys objectAtIndex:i];
    }
    return @"Unknown";
}
-(void)setType:(NSString*)s
{
    [self galaxy]->setType((Galaxy::GalaxyType)[[galaxyTypeDict objectForKey:s] intValue]);
}
-(NSString *)name
{
    return [NSString stringWithStdString:[self galaxy]->getName()];
}
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
-(void)setName:(NSString*)s
{
    [self galaxy]->setName([s stdString]);
}
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