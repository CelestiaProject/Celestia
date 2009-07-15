// boundaries.h
//
// Copyright (C) 2002-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_BOUNDARIES_H_
#define _CELENGINE_BOUNDARIES_H_

#include <Eigen/Core>
#include <string>
#include <vector>
#include <iostream>

class ConstellationBoundaries
{
 public:
    ConstellationBoundaries();
    ~ConstellationBoundaries();

    typedef std::vector<Eigen::Vector3f> Chain;

    void moveto(float ra, float dec);
    void lineto(float ra, float dec);
    void render();

 private:
    Chain* currentChain;
    std::vector<Chain*> chains;
};

ConstellationBoundaries* ReadBoundaries(std::istream&);

#endif // _CELENGINE_BOUNDARIES_H_
