// boundaries.h
//
// Copyright (C) 2002-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_BOUNDARIES_H_
#define _CELENGINE_BOUNDARIES_H_

#include <Eigen/Core>
#include <vector>
#include <iostream>

class ConstellationBoundaries
{
 public:
    using Chain = std::vector<Eigen::Vector3f>;

    ConstellationBoundaries();
    ~ConstellationBoundaries();
    ConstellationBoundaries(const ConstellationBoundaries&)            = delete;
    ConstellationBoundaries(ConstellationBoundaries&&)                 = delete;
    ConstellationBoundaries& operator=(const ConstellationBoundaries&) = delete;
    ConstellationBoundaries& operator=(ConstellationBoundaries&&)      = delete;

    void moveto(float ra, float dec);
    void lineto(float ra, float dec);

    const std::vector<Chain*>& getChains() const;

 private:
    Chain* currentChain{ nullptr };
    std::vector<Chain*> chains;
};

ConstellationBoundaries* ReadBoundaries(std::istream&);

#endif // _CELENGINE_BOUNDARIES_H_
