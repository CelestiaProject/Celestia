// boundaries.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_BOUNDARIES_H_
#define CELENGINE_BOUNDARIES_H_

#include <string>
#include <vector>
#include <iostream>
#include <celmath/vecmath.h>

class ConstellationBoundaries
{
 public:
    ConstellationBoundaries();
    ~ConstellationBoundaries();

    typedef std::vector<Point3f> Chain;

    void moveto(float ra, float dec);
    void lineto(float ra, float dec);
    void render();

 private:
    Chain* currentChain;
    std::vector<Chain*> chains;
};

ConstellationBoundaries* ReadBoundaries(std::istream&);

#endif // CELENGINE_BOUNDARIES_H_
