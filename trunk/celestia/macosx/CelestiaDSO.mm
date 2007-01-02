//
//  CelestiaDSO.mm
//  celestia
//
//  Created by Da Woon Jung on 12/30/06.
//  Copyright 2006 Chris Laurel. All rights reserved.
//

#import "CelestiaDSO.h"
#import "CelestiaAppCore.h"
#import "CelestiaAppCore_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"

@implementation CelestiaDSO

-(id)initWithDSO:(DeepSkyObject*)aDSO
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&aDSO) objCType:@encode(DeepSkyObject*)];
    return self;
}

-(DeepSkyObject*)DSO
{
    return reinterpret_cast<DeepSkyObject*>([_data pointerValue]);
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
    return [NSString stringWithStdString: [self DSO]->getType()];
}

-(NSString *)name
{
    return [NSString stringWithStdString:[[CelestiaAppCore sharedAppCore] appCore]->getSimulation()->getUniverse()->getDSOCatalog()->getDSOName([self DSO])];
}
@end
