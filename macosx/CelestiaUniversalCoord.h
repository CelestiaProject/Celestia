//
//  CelestiaUniversalCoord.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "CelestiaVector.h"
#import "CelestiaUniversalCoord.h"

@interface CelestiaUniversalCoord : NSObject {
    NSData* _data;
}
-(CelestiaVector*)celestiaVector;
-(NSNumber*)distanceTo:(CelestiaUniversalCoord*)t;
-(CelestiaUniversalCoord*)difference:(CelestiaUniversalCoord*)t;
@end
