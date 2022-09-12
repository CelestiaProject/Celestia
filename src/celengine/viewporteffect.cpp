//
// viewporteffect.cpp
//
// Copyright © 2020 Celestia Development Team. All rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "viewporteffect.h"
#include "framebuffer.h"
#include "render.h"
#include "shadermanager.h"
#include "mapmanager.h"

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
    ViewportEffect(),
    vo(GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW)
{
}

bool PassthroughViewportEffect::render(Renderer* renderer, FramebufferObject* fbo, int width, int height)
{
    CelestiaGLProgram *prog = renderer->getShaderManager().getShader("passthrough");
    if (prog == nullptr)
        return false;

    vo.bind();
    if (!vo.initialized())
        initializeVO(vo);

    prog->use();
    prog->samplerParam("tex") = 0;
    glBindTexture(GL_TEXTURE_2D, fbo->colorTexture());
    renderer->setPipelineState(ps);
    draw(vo);
    glBindTexture(GL_TEXTURE_2D, 0);
    vo.unbind();
    return true;
}

void PassthroughViewportEffect::initializeVO(celgl::VertexObject& vo)
{
    static float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    vo.allocate(sizeof(quadVertices), quadVertices);
    vo.setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                            2, GL_FLOAT, false, 4 * sizeof(float), 0);
    vo.setVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex,
                            2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float));
}

void PassthroughViewportEffect::draw(celgl::VertexObject& vo)
{
    vo.draw(GL_TRIANGLES, 6);
}

WarpMeshViewportEffect::WarpMeshViewportEffect(WarpMesh *mesh) :
    ViewportEffect(),
    vo(GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW),
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
    CelestiaGLProgram *prog = renderer->getShaderManager().getShader("warpmesh");
    if (prog == nullptr)
        return false;

    vo.bind();
    if (!vo.initialized())
        initializeVO(vo);

    prog->use();
    prog->samplerParam("tex") = 0;
    prog->floatParam("screenRatio") = (float)height / width;
    glBindTexture(GL_TEXTURE_2D, fbo->colorTexture());
    renderer->setPipelineState(ps);
    draw(vo);
    glBindTexture(GL_TEXTURE_2D, 0);
    vo.unbind();
    return true;
}

void WarpMeshViewportEffect::initializeVO(celgl::VertexObject& vo)
{
    mesh->scopedDataForRendering([&vo](float *data, int size){
        vo.allocate(size, data);
        vo.setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                                2, GL_FLOAT, false, 5 * sizeof(float), 0);
        vo.setVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex,
                                2, GL_FLOAT, false, 5 * sizeof(float), 2 * sizeof(float));
        vo.setVertexAttribArray(CelestiaGLProgram::IntensityAttributeIndex,
                                1, GL_FLOAT, false, 5 * sizeof(float), 4 * sizeof(float));
    });
}

void WarpMeshViewportEffect::draw(celgl::VertexObject& vo)
{
    vo.draw(GL_TRIANGLES, mesh->count());
}

bool WarpMeshViewportEffect::distortXY(float &x, float &y)
{
    if (mesh == nullptr)
        return false;

    float u;
    float v;
    if (!mesh->mapVertex(x * 2, y * 2, &u, &v))
        return false;

    x = u / 2;
    y = v / 2;
    return true;
}
