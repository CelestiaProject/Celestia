// vertexlist.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <GL/glew.h>
#include "vecgl.h"
#include "vertexlist.h"

using namespace Eigen;
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
    // HACK: Don't delete the vertex data; the VertexList class as an intermediate
    // step in converting from 3DS models to Celestia models, and after the
    // conversion, the Celestia model will own the vertex data pointer.
#if 0
    if (vertices != NULL)
        delete[] vertices;
#endif
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
    vertices[n++].f = v.point.x();
    vertices[n++].f = v.point.y();
    vertices[n++].f = v.point.z();
    if ((parts & VertexNormal) != 0)
    {
        vertices[n++].f = v.normal.x();
        vertices[n++].f = v.normal.y();
        vertices[n++].f = v.normal.z();
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
        vertices[n++].f = v.texCoords[0].x();
        vertices[n++].f = v.texCoords[0].y();
    }
    if ((parts & TexCoord1) != 0)
    {
        vertices[n++].f = v.texCoords[1].x();
        vertices[n++].f = v.texCoords[1].y();
    }

    bbox.extend(v.point);

    nVertices++;
}


Eigen::AlignedBox<float, 3> VertexList::getBoundingBox() const
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
void VertexList::transform(const Vector3f& translation, float scale)
{
    for (uint32 i = 0; i < nVertices; i++)
    {
        uint32 n = i * vertexSize;
        Vector3f tv = (Vector3f(vertices[n].f, vertices[n + 1].f, vertices[n + 2].f) + translation) * scale;
        vertices[n    ].f = tv.x();
        vertices[n + 1].f = tv.y();
        vertices[n + 2].f = tv.z();
    }

    // Transform the bounding box
    bbox.translate(translation);
    bbox = AlignedBox<float, 3>(bbox.min() * scale, bbox.max() * scale);
}


uint32 VertexList::getVertexParts() const
{
    return parts;
}


void* VertexList::getVertexData() const
{
    return reinterpret_cast<void*>(vertices);
}


uint32 VertexList::getVertexCount() const
{
    return nVertices;
}
