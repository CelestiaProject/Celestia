/*
 *  CelestiaGalaxy_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Sat Jun 08 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celengine/galaxy.h>
@interface CelestiaGalaxy(PrivateAPI)
-(CelestiaGalaxy*)initWithGalaxy:(Galaxy*)g;
-(Galaxy*)galaxy;
@end
