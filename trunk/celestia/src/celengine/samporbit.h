// samporbit.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SAMPORBIT_H_
#define _CELENGINE_SAMPORBIT_H_

#include <string>
#include <celengine/orbit.h>

extern Orbit* LoadSampledOrbit(const std::string& name);

#endif // _CELENGINE_SAMPORBIT_H_
