// parseobject.h
//
// Copyright (C) 2004 Chris Laurel <claurel@shatters.net>
//
// Functions for parsing objects common to star, solar system, and
// deep sky catalogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_PARSEOBJECT_H_
#define _CELENGINE_PARSEOBJECT_H_

#include <string>
#include "astro.h"
#include "body.h"
#include "parser.h"

bool ParseDate(Hash* hash, const string& name, double& jd);

Orbit* CreateOrbit(PlanetarySystem* system,
                   Hash* planetData,
                   const std::string& path,
                   bool usePlanetUnits);

RotationModel* CreateRotationModel(Hash* rotationData,
                                   const string& path,
                                   float syncRotationPeriod);

RotationModel* CreateDefaultRotationModel(double syncRotationPeriod);

#endif // _CELENGINE_PARSEOBJECT_H_
