/*
 *  CelestiaSimulation_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celengine/simulation.h>

@interface CelestiaSimulation(PrivateAPI)
-(CelestiaSimulation*)initWithSimulation:(Simulation*)sim;
-(Simulation*)simulation;
@end