// dispmap.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _DISPMAP_H_
#define _DISPMAP_H_

#include <string>
#include <celmath/vecmath.h>


typedef float (*DisplacementMapFunc)(float, float, void*);

class DisplacementMap
{
 public:
    DisplacementMap(int w, int h); 
    ~DisplacementMap();
    int getWidth() const { return width; };
    int getHeight() const { return height; };
    inline float getDisplacement(int x, int y) const;
    inline void setDisplacement(int x, int y, float d);
    void generate(DisplacementMapFunc func, void* info = NULL);
    void clear();
    
 private:
    int width;
    int height;
    float* disp;
};

// extern DisplacementMap* LoadDisplacementMap(std::string filename);


float DisplacementMap::getDisplacement(int x, int y) const
{
    return disp[y * width + x];
}

void DisplacementMap::setDisplacement(int x, int y, float d)
{
    disp[y * width + x] = d;
}

#endif // _DISPMAP_H_
