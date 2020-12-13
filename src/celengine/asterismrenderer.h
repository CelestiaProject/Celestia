// asterismrenderer.h
//
// Copyright (C) 2018-2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>
#include <celengine/asterism.h>
#include <celutil/color.h>
#include "shadermanager.h"
#include "vertexobject.h"

class Renderer;
struct Matrices;
struct LineEnds;

class AsterismRenderer
{
 public:
    AsterismRenderer(const AsterismList *asterisms);
    ~AsterismRenderer() = default;
    AsterismRenderer() = delete;
    AsterismRenderer(const AsterismRenderer&) = delete;
    AsterismRenderer(AsterismRenderer&&) = delete;
    AsterismRenderer& operator=(const AsterismRenderer&) = delete;
    AsterismRenderer& operator=(AsterismRenderer&&) = delete;

    void render(const Renderer &renderer, const Color &color, const Matrices &mvp);
    bool sameAsterisms(const AsterismList *asterisms) const;

 private:
    bool prepare(std::vector<LineEnds> &data);

    celgl::VertexObject     m_vo        { GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW };
    ShaderProperties        m_shadprop;
    std::vector<GLsizei>    m_lineCount;

    const AsterismList     *m_asterisms { nullptr };
    GLsizei                 m_totalLineCount  { 0 };
};
