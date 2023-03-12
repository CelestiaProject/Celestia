//
// viewporteffect.cpp
//
// Copyright © 2020 Celestia Development Team. All rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include "viewporteffect.h"
#include "framebuffer.h"
#include "render.h"
#include "shadermanager.h"
#include "mapmanager.h"

namespace gl = celestia::gl;
namespace util = celestia::util;

static const Renderer::PipelineState ps;

bool ViewportEffect::preprocess(Renderer* renderer, FramebufferObject* fbo)
{
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFboId);
    return fbo->bind();
}

bool ViewportEffect::prerender(Renderer* renderer, FramebufferObject* fbo)
{
    if (!fbo->unbind(oldFboId))
        return false;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return true;
}

bool ViewportEffect::distortXY(float &x, float &y)
{
    return true;
}

PassthroughViewportEffect::PassthroughViewportEffect() :
    ViewportEffect()
{
}

bool PassthroughViewportEffect::render(Renderer* renderer, FramebufferObject* fbo, int width, int height)
{
    auto *prog = renderer->getShaderManager().getShader("passthrough");
    if (prog == nullptr)
        return false;

    initialize();

    prog->use();
    prog->samplerParam("tex") = 0;
    glBindTexture(GL_TEXTURE_2D, fbo->colorTexture());
    renderer->setPipelineState(ps);
    vo.draw();
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void PassthroughViewportEffect::initialize()
{
    if (initialized)
        return;
    initialized = true;

    static std::array quadVertices = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    vo = gl::VertexObject();
    bo = gl::Buffer(gl::Buffer::TargetHint::Array, quadVertices, gl::Buffer::BufferUsage::StaticDraw);

    vo.setCount(6);
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        4 * sizeof(float),
        0);
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        4 * sizeof(float),
        2 * sizeof(float));
}

WarpMeshViewportEffect::WarpMeshViewportEffect(WarpMesh *mesh) :
    ViewportEffect(),
    mesh(mesh)
{
}

bool WarpMeshViewportEffect::prerender(Renderer* renderer, FramebufferObject* fbo)
{
    if (mesh == nullptr)
        return false;

    return ViewportEffect::prerender(renderer, fbo);
}

bool WarpMeshViewportEffect::render(Renderer* renderer, FramebufferObject* fbo, int width, int height)
{
    auto *prog = renderer->getShaderManager().getShader("warpmesh");
    if (prog == nullptr)
        return false;

    initialize();

    prog->use();
    prog->samplerParam("tex") = 0;
    prog->floatParam("screenRatio") = (float)height / width;
    glBindTexture(GL_TEXTURE_2D, fbo->colorTexture());
    renderer->setPipelineState(ps);
    vo.draw();
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void WarpMeshViewportEffect::initialize()
{
    if (initialized)
        return;
    initialized = true;

    vo = gl::VertexObject();
    bo = gl::Buffer();

    bo.bind().setData(mesh->scopedDataForRendering(), gl::Buffer::BufferUsage::StaticDraw);

    vo.setCount(mesh->count());
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        5 * sizeof(float),
        0);
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        5 * sizeof(float),
        2 * sizeof(float));
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::IntensityAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        5 * sizeof(float),
        4 * sizeof(float));
}

bool WarpMeshViewportEffect::distortXY(float &x, float &y)
{
    if (mesh == nullptr)
        return false;

    float u;
    float v;
    if (!mesh->mapVertex(x * 2.0f, y * 2.0f, &u, &v))
        return false;

    x = u / 2.0f;
    y = v / 2.0f;
    return true;
}
