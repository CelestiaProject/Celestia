// imagecapture.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celcompat/filesystem.h>
#include <celutil/filetype.h>

class Renderer;

bool CaptureBufferToFile(const fs::path& filename,
                         int x, int y,
                         int width, int height,
                         const Renderer *renderer,
                         ContentType type);
