/*
 *  CelestiaStar_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Sat Jun 08 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celengine/star.h>
@interface CelestiaStar(PrivateAPI)
-(CelestiaStar*)initWithStar:(const Star*)s;
-(Star*)star;
@end
