// samporient.h
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SAMPORIENT_H_
#define _CELENGINE_SAMPORIENT_H_

#include <string>
#include <celengine/rotation.h>

extern RotationModel* LoadSampledOrientation(const std::string& name);

#endif // _CELENGINE_SAMPORIENT_H_
