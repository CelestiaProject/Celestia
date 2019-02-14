//
//  CelestiaLocation.mm
//  celestia
//
//  Created by Da Woon Jung on 12/31/06.
//  Copyright 2006 Chris Lauerl. All rights reserved.
//

#import "CelestiaLocation.h"
#import "NSString_ObjCPlusPlus.h"


@implementation CelestiaLocation

-(id)initWithLocation:(Location*)aLocation
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&aLocation) objCType:@encode(Location*)];
    return self;
}

-(Location*)location
{
    return reinterpret_cast<Location*>([_data pointerValue]);
}

-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}

-(NSString*)name
{
    return [NSString stringWithStdString: [self location]->getName(true)];
}
@end
