/*
 *  CelestiaUniverse_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celengine/universe.h>

@interface CelestiaUniverse(PrivateAPI)
-(CelestiaUniverse*)initWithUniverse:(Universe*)uni;
-(Universe*)universe;
@end