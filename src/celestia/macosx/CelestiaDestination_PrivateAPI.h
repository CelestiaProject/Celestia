/*
 *  CelestiaDestination_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celestia/destination.h>

@interface CelestiaDestination(PrivateAPI)
-(CelestiaDestination*)initWithDestination:(Destination*)dst;
-(Destination*)destination;
@end

@interface CelestiaDestinations(PrivateAPI)
-(CelestiaDestinations*)initWithDestinations:(const DestinationList*)dsts;
-(DestinationList*)destinations;
@end