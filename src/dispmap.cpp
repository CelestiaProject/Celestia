// dispmap.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "dispmap.h"


DisplacementMap::DisplacementMap(int w, int h) :
    width(w), height(h), disp(NULL)
{
    disp = new float[width * height];
}

DisplacementMap::~DisplacementMap()
{
    if (disp != NULL)
        delete[] disp;
}


void DisplacementMap::clear()
{
    int size = width * height;
    for (int i = 0; i < size; i++)
        disp[i] = 0.0f;
}


void DisplacementMap::generate(DisplacementMapFunc func, void* info)
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            disp[i * width + j] = func((float) j / (float) width,
                                       (float) i / (float) height, info);
        }
    }
}
