// customrotation.h
//
// Custom rotation models for Solar System bodies.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_CUSTOMROTATION_H_
#define _CELENGINE_CUSTOMROTATION_H_

#include <string>

class RotationModel;

RotationModel* GetCustomRotationModel(const std::string& name);

#endif // _CELENGINE_CUSTOMROTATION_H_

