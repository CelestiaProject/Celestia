//
//  CelestiaRenderer.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaRenderer.h"
#include <celengine/render.h>

@implementation CelestiaRenderer(PrivateAPI)
-(CelestiaRenderer *)initWithRenderer:(Renderer *)ren 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&ren) objCType:@encode(Renderer*)];
    return self;
}

-(Renderer *)renderer
{
    return reinterpret_cast<Renderer*>([_data pointerValue]);
}
@end

@implementation CelestiaRenderer
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
@end
