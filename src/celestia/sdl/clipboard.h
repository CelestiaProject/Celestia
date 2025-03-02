// clipboard.h
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

class CelestiaCore;

namespace celestia::sdl
{

void doCopy(CelestiaCore& appCore);
void doPaste(CelestiaCore& appCore);

}
