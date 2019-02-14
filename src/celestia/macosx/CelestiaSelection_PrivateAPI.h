/*
 *  CelestiaSelection_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#import "CelestiaUniversalCoord_PrivateAPI.h"
#include <celengine/selection.h>

@interface CelestiaSelection(PrivateAPI)
-(CelestiaSelection*)initWithSelection:(Selection)selection;
-(Selection)selection;
@end