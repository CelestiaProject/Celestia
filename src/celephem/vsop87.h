// vsop87.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_VSOP87_H_
#define _CELENGINE_VSOP87_H_

#include <string>
#include "orbit.h"

extern Orbit* CreateVSOP87Orbit(const std::string& name);

#endif // _CELENGINE_VSOP87_H_
