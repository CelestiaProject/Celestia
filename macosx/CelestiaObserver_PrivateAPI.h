/*
 *  CelestiaObserver_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */
 
#include <celengine/observer.h>

@interface CelestiaObserver(PrivateAPI)
-(Observer)observer;
-(CelestiaObserver*)initWithObserver:(Observer)obs;
@end