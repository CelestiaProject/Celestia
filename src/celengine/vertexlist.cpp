// vertexlist.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "gl.h"
#include "glext.h"
#include "vecgl.h"
#include "vertexlist.h"

using namespace std;


VertexList::VertexList(uint32 _parts, uint32 initialVertexPoolSize) :
    parts(_parts),
    nVertices(0),
    maxVertices(0),
    vertices(NULL),
    diffuseColor(1.0f, 1.0f, 1.0f),
    specularColor(0.0f, 0.0f, 0.0f),
    shininess(0.0f),
    texture(InvalidResource),
    bbox()
{
    if (initialVertexPoolSize > 0)
    {
        maxVertices = initialVertexPoolSize;
        vertices = new VertexPart[vertexSize * maxVertices];
    }

    vertexSize = 3;
    if ((parts & VertexNormal) != 0)
        vertexSize += 3;
    if ((parts & VertexColor0) != 0)
        vertexSize += 1;
    if ((parts & TexCoord0) != 0)
        vertexSize += 2;
    if ((parts & TexCoord1) != 0)
        vertexSize += 2;
}


VertexList::~VertexList()
{
    if (vertices != NULL)
        delete[] vertices;
}


void VertexList::render()
{
    GLsizei stride = sizeof(VertexPart) * vertexSize;
    uint32 start = 3;

    // Vertex points
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, static_cast<void*>(vertices));
    
    // Vertex normals
    if ((parts & VertexNormal) != 0)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, stride, static_cast<void*>(&vertices[start]));
        start += 3;
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    // Vertex color
    if ((parts & VertexColor0) != 0)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, stride,
                       static_cast<void*>(&vertices[start]));
        start += 1;
    }
    else
    {
        glDisableClientState(GL_COLOR_ARRAY);
        glColor(diffuseColor);
    }

    // Texture coordinates
    if ((parts & TexCoord0) != 0)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride,
                          static_cast<void*>(&vertices[start]));
        start += 2;
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if ((parts & TexCoord1) != 0)
    {
        // glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride,
                          static_cast<void*>(&vertices[start]));
        start += 2;
    }

    glDrawArrays(GL_TRIANGLES, 0, nVertices);
}


void VertexList::addVertex(const Vertex& v)
{
    if (nVertices == maxVertices)
    {
        if (maxVertices == 0)
        {
            vertices = new VertexPart[16 * vertexSize];
            maxVertices = 16;
        }
        else
        {
            VertexPart* newVertices = new VertexPart[maxVertices * 2 * vertexSize];
            copy(vertices, vertices + nVertices * vertexSize, newVertices);
            delete[] vertices;
            vertices = newVertices;
            maxVertices *= 2;
        }
    }

    uint32 n = nVertices * vertexSize;
    vertices[n++].f = v.point.x;
    vertices[n++].f = v.point.y;
    vertices[n++].f = v.point.z;
    if ((parts & VertexNormal) != 0)
    {
        vertices[n++].f = v.normal.x;
        vertices[n++].f = v.normal.y;
        vertices[n++].f = v.normal.z;
    }
    if ((parts & VertexColor0) != 0)
    {
        vertices[n].c[0] = (int) (v.color.red() * 255.99f);
        vertices[n].c[1] = (int) (v.color.green() * 255.99f);
        vertices[n].c[2] = (int) (v.color.blue() * 255.99f);
        vertices[n].c[3] = (int) (v.color.alpha() * 255.99f);
        n++;
    }
    if ((parts & TexCoord0) != 0)
    {
        vertices[n++].f = v.texCoords[0].x;
        vertices[n++].f = v.texCoords[0].y;
    }
    if ((parts & TexCoord1) != 0)
    {
        vertices[n++].f = v.texCoords[1].x;
        vertices[n++].f = v.texCoords[1].y;
    }

    bbox.include(v.point);

    nVertices++;
}


AxisAlignedBox VertexList::getBoundingBox() const
{
    return bbox;
}


Color VertexList::getDiffuseColor() const
{
    return diffuseColor;
}

void VertexList::setDiffuseColor(Color color)
{
    diffuseColor = color;
}

Color VertexList::getSpecularColor() const
{
    return specularColor;
}

void VertexList::setSpecularColor(Color color)
{
    specularColor = color;
}

float VertexList::getShininess() const
{
    return shininess;
}

void VertexList::setShininess(float _shininess)
{
    shininess = _shininess;
}

ResourceHandle VertexList::getTexture() const
{
    return texture;
}

void VertexList::setTexture(ResourceHandle _texture)
{
    texture = _texture;
}


// Apply a translation and uniform scale to the vertices
void VertexList::transform(Vec3f translation, float scale)
{
    for (uint32 i = 0; i < nVertices; i++)
    {
        uint32 n = i * vertexSize;
        Vec3f tv = (Vec3f(vertices[n].f, vertices[n + 1].f, vertices[n + 2].f) + translation) * scale;
        vertices[n    ].f = tv.x;
        vertices[n + 1].f = tv.y;
        vertices[n + 2].f = tv.z;
    }

    // Transform the bounding box
    Point3f mn = bbox.getMinimum();
    Point3f mx = bbox.getMaximum();
    Point3f tr(-translation.x, -translation.y, -translation.z);
    bbox = AxisAlignedBox(Point3f(0, 0, 0) + ((mn - tr) * scale),
                          Point3f(0, 0, 0) + ((mx - tr) * scale));
}
