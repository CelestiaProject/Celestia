//
//  CelestiaDSO.h
//  celestia
//
//  Created by Da Woon Jung on 12/30/06.
//  Copyright 2006 Chris Laurel. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "deepskyobj.h"

@interface CelestiaDSO : NSObject
{
    NSValue *_data;
}
-(id)initWithDSO:(DeepSkyObject*)aDSO;
-(DeepSkyObject*)DSO;

-(NSString *)name;
-(NSString*)type;
@end
