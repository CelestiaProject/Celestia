// solarsysxml.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SOLARSYSXML_H_
#define _CELENGINE_SOLARSYSXML_H_

#include <string>
#include "universe.h"


extern bool LoadSolarSystemObjectsXML(const std::string& source,
                                      Universe& universe);

#endif // _CELENGINE_SOLARSYSXML_H_
