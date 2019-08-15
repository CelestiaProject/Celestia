// spiceinterface.cpp
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SPICEINTERFACE_H_
#define _CELENGINE_SPICEINTERFACE_H_

#include <string>
#include <celcompat/filesystem.h>

extern bool InitializeSpice();

// SPICE utility functions

extern bool GetNaifId(const std::string& name, int* id);
extern bool IsSpiceKernelLoaded(const fs::path& filepath);
extern bool LoadSpiceKernel(const fs::path& filepath);

#endif // _CELENGINE_SPICEINTERFACE_H_
