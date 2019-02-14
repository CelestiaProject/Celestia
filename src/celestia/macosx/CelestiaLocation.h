//
//  CelestiaLocation.h
//  celestia
//
//  Created by Da Woon Jung on 12/31/06.
//  Copyright 2006 Chris Laurel. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "location.h"

@interface CelestiaLocation : NSObject
{
    NSValue *_data;
}
-(id)initWithLocation:(Location*)aLocation;
-(Location*)location;

-(NSString*)name;
@end
