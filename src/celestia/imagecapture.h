// imagecapture.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _IMAGECAPTURE_H_
#define _IMAGECAPTURE_H_

#include <celcompat/filesystem.h>
#include <celengine/render.h>


extern bool CaptureGLBufferToJPEG(const fs::path& filename,
                                  int x, int y,
                                  int width, int height,
                                  const Renderer *renderer);
extern bool CaptureGLBufferToPNG(const fs::path& filename,
                                 int x, int y,
                                 int width, int height,
                                 const Renderer *renderer);

#endif // _IMAGECAPTURE_H_
