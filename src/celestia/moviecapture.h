// moviecapture.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _MOVIECAPTURE_H_
#define _MOVIECAPTURE_H_

#include <string>


class MovieCapture
{
 public:
    MovieCapture() {};
    virtual ~MovieCapture() {};

    virtual bool start(const std::string& filename, int width, int height) = 0;
    virtual bool end() = 0;
    virtual bool captureFrame() = 0;
};

#endif // _MOVIECAPTURE_H_

