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

    void render(const Renderer &renderer, const Color &color, const Eigen::Matrix4f &mvp);
    bool sameAsterisms(const AsterismList *asterisms) const;

 private:
    GLfloat* prepare();

    celgl::VertexObject     m_vo        { GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW };
    ShaderProperties        m_shadprop;
    std::vector<GLsizei>    m_vtxCount;

    const AsterismList     *m_asterisms { nullptr };
    GLsizei                 m_vtxTotal  { 0 };
};
