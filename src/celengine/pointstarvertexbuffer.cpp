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

void PointStarVertexBuffer::startSprites()
{
    auto *prog = renderer.getShaderManager().getShader("star");
    if (prog == nullptr)
        return;
    prog->use();
    prog->setMVPMatrices(renderer.getProjectionMatrix(), renderer.getModelViewMatrix());
    prog->samplerParam("starTex") = 0;

    unsigned int stride = sizeof(StarVertex);
    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          stride, &vertices[0].position);
    glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                          4, GL_UNSIGNED_BYTE, GL_TRUE,
                          stride, &vertices[0].color);

    glEnableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                          1, GL_FLOAT, GL_FALSE,
                          stride, &vertices[0].size);

#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
#endif

    useSprites = true;
}

void PointStarVertexBuffer::startPoints()
{
    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;
    auto *prog = renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;
    prog->use();
    prog->setMVPMatrices(renderer.getProjectionMatrix(), renderer.getModelViewMatrix());

    unsigned int stride = sizeof(StarVertex);
    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          stride, &vertices[0].position);
    glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                          4, GL_UNSIGNED_BYTE, GL_TRUE,
                          stride, &vertices[0].color);

    // An option to control the size of the stars would be helpful.
    // Which size looks best depends a lot on the resolution and the
    // type of display device.
    // glPointSize(2.0f);
    // glEnable(GL_POINT_SMOOTH);
    useSprites = false;
}

void PointStarVertexBuffer::render()
{
    if (nStars != 0)
    {
        unsigned int stride = sizeof(StarVertex);
#ifndef GL_ES
        if (useSprites)
        {
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        }
        else
        {
            glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glPointSize(1.0f);
        }
#endif
        glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                              3, GL_FLOAT, GL_FALSE,
                              stride, &vertices[0].position);
        glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                              4, GL_UNSIGNED_BYTE, GL_TRUE,
                              stride, &vertices[0].color);

        if (useSprites)
        {
            glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                                  1, GL_FLOAT, GL_FALSE,
                                  stride, &vertices[0].size);
        }

        if (texture != nullptr)
            texture->bind();
        glDrawArrays(GL_POINTS, 0, nStars);
        nStars = 0;
    }
}

void PointStarVertexBuffer::finish()
{
    render();
    glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);

    if (useSprites)
    {
        glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
#ifndef GL_ES
        glDisable(GL_POINT_SPRITE);
#endif
    }
}

void PointStarVertexBuffer::setTexture(Texture* _texture)
{
  texture = _texture;
}
