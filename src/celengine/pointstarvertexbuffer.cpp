// starfield.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "glsupport.h"
#include <celutil/color.h>
#include "objectrenderer.h"
#include "shadermanager.h"
#include "render.h"
#include "texture.h"
#include "pointstarvertexbuffer.h"

PointStarVertexBuffer* PointStarVertexBuffer::current = nullptr;

PointStarVertexBuffer::PointStarVertexBuffer(const Renderer& _renderer,
                                             unsigned int _capacity) :
    renderer(_renderer),
    capacity(_capacity)
{
    vertices = new StarVertex[capacity];
}

PointStarVertexBuffer::~PointStarVertexBuffer()
{
    delete[] vertices;
}

void PointStarVertexBuffer::startSprites(float _limitingMagnitude)
{
    program = renderer.getShaderManager().getShader("star_new");
    pointSizeFromVertex = true;
    limitingMagnitude = _limitingMagnitude;
}

void PointStarVertexBuffer::startBasicPoints()
{
    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors | ShaderProperties::StaticPointSize;
    shadprop.lightModel = ShaderProperties::UnlitModel;
    program = renderer.getShaderManager().getShader(shadprop);
    pointSizeFromVertex = false;
}

void PointStarVertexBuffer::render()
{
    if (nStars != 0)
    {
        makeCurrent();
        unsigned int stride = sizeof(StarVertex);
        glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                              3, GL_FLOAT, GL_FALSE,
                              stride, &vertices[0].position);
        glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                              4, GL_UNSIGNED_BYTE, GL_TRUE,
                              stride, &vertices[0].color);

        if (pointSizeFromVertex)
            glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                                  1, GL_FLOAT, GL_FALSE,
                                  stride, &vertices[0].size);

        if (texture != nullptr)
            texture->bind();
        glDrawArrays(GL_POINTS, 0, nStars);
        nStars = 0;
    }
}

void PointStarVertexBuffer::makeCurrent()
{
    if (current == this || program == nullptr)
        return;

    program->use();
    program->setMVPMatrices(renderer.getProjectionMatrix(), renderer.getModelViewMatrix());
    std::array<int, 4> viewport;
    renderer.getViewport(viewport);
    program->vec2Param("viewportSize") = Eigen::Vector2f(viewport[2], viewport[3]);
    program->vec2Param("viewportCoord") = Eigen::Vector2f(viewport[0], viewport[1]);
    float visibilityThreshold = 1.0f / 255.0f;
    float logMVisThreshold = log(visibilityThreshold) / log(2.512f);
    float saturationMag = limitingMagnitude - 4.5f/* + logMVisThreshold*/;
    float magScale = (logMVisThreshold) / (saturationMag - limitingMagnitude);
    program->floatParam("thresholdBrightness") = visibilityThreshold;
    program->floatParam("exposure") = pow(2.512f, magScale * saturationMag);
    program->floatParam("magScale") = magScale;

    if (pointSizeFromVertex)
    {
        program->samplerParam("starTex") = 0;
        glEnableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
    }
    else
    {
        program->pointScale = pointScale;
        glVertexAttrib1f(CelestiaGLProgram::PointSizeAttributeIndex, 1.0f);
    }

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    current = this;
}

void PointStarVertexBuffer::finish()
{
    render();
    glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    if (pointSizeFromVertex)
        glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
}

void PointStarVertexBuffer::enable()
{
#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
}

void PointStarVertexBuffer::disable()
{
#ifndef GL_ES
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SPRITE);
#endif
}

void PointStarVertexBuffer::setTexture(Texture* _texture)
{
    texture = _texture;
}

void PointStarVertexBuffer::setPointScale(float pointSize)
{
    pointScale = pointSize;
}
