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

#include <string>


extern bool CaptureGLBufferToJPEG(const std::string& filename,
                                  int x, int y,
                                  int width, int height);
extern bool CaptureGLBufferToPNG(const std::string& filename,
                                 int x, int y,
                                 int width, int height);

#endif // _IMAGECAPTURE_H_
