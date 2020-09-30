// boundariesrenderer.h
//
// Copyright (C) 2018-2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "shadermanager.h"
#include "vertexobject.h"

class Color;
class ConstellationBoundaries;
class Renderer;
struct Matrices;

class BoundariesRenderer
{
 public:
    BoundariesRenderer(const ConstellationBoundaries*);
    ~BoundariesRenderer() = default;
    BoundariesRenderer() = delete;
    BoundariesRenderer(const BoundariesRenderer&) = delete;
    BoundariesRenderer(BoundariesRenderer&&) = delete;
    BoundariesRenderer& operator=(const BoundariesRenderer&) = delete;
    BoundariesRenderer& operator=(BoundariesRenderer&&) = delete;

    void render(const Renderer &renderer, const Color &color, const Matrices &mvp);
    bool sameBoundaries(const ConstellationBoundaries*) const;

 private:
    GLshort* prepare();

    celgl::VertexObject            m_vo         { GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW };
    ShaderProperties               m_shadprop;
    const ConstellationBoundaries *m_boundaries { nullptr };
    GLsizei                        m_vtxTotal   { 0 };
};
