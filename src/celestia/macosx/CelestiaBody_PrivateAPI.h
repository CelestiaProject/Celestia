/*
 *  CelestiaBody_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Sat Jun 08 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celengine/body.h>
@interface CelestiaBody(PrivateAPI)
-(CelestiaBody*)initWithBody:(Body*)b;
-(Body*)body;
@end
