// starfield.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <GL/glew.h>
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
    prog->samplerParam("starTex") = 0;

    unsigned int stride = sizeof(StarVertex);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, &vertices[0].position);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, &vertices[0].color);

    glEnableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                          1, GL_FLOAT, GL_FALSE,
                          stride, &vertices[0].size);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glEnable(GL_POINT_SPRITE);

    useSprites = true;
}

void PointStarVertexBuffer::startPoints()
{
    auto *prog = renderer.getShaderManager().getShader(ShaderProperties::PerVertexColor);
    if (prog == nullptr)
        return;
    prog->use();

    unsigned int stride = sizeof(StarVertex);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, &vertices[0].position);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, &vertices[0].color);

    // An option to control the size of the stars would be helpful.
    // Which size looks best depends a lot on the resolution and the
    // type of display device.
    // glPointSize(2.0f);
    // glEnable(GL_POINT_SMOOTH);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    useSprites = false;
}

void PointStarVertexBuffer::render()
{
    if (nStars != 0)
    {
        unsigned int stride = sizeof(StarVertex);
        if (useSprites)
        {
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        }
        else
        {
            glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glPointSize(1.0f);
        }
        glVertexPointer(3, GL_FLOAT, stride, &vertices[0].position);
        glColorPointer(4, GL_UNSIGNED_BYTE, stride, &vertices[0].color);

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
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if (useSprites)
    {
        glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
        glDisable(GL_POINT_SPRITE);
    }
    glUseProgram(0);
}

void PointStarVertexBuffer::setTexture(Texture* _texture)
{
  texture = _texture;
}
