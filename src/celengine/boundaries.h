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
#include "celutil/color.h"
#include "shadermanager.h"

class Renderer;

class ConstellationBoundaries
{
    using Chain = std::vector<Eigen::Vector3f>;

 public:
    ConstellationBoundaries();
    ~ConstellationBoundaries();
    ConstellationBoundaries(const ConstellationBoundaries&)            = delete;
    ConstellationBoundaries(ConstellationBoundaries&&)                 = delete;
    ConstellationBoundaries& operator=(const ConstellationBoundaries&) = delete;
    ConstellationBoundaries& operator=(ConstellationBoundaries&&)      = delete;

    void moveto(float ra, float dec);
    void lineto(float ra, float dec);
    void render(const Color& color, const Renderer& renderer);

 private:
    void cleanup();
    void prepare();

    Chain* currentChain{ nullptr };
    std::vector<Chain*> chains;

    GLuint  vboId{ 0 };
    GLshort *vtx_buf{ nullptr };
    GLsizei vtx_num{ 0 };
    bool prepared{ false };

    ShaderProperties shadprop;
};

ConstellationBoundaries* ReadBoundaries(std::istream&);

#endif // _CELENGINE_BOUNDARIES_H_
