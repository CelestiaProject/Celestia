// vertexbuf.h
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _VERTEXBUF_H_
#define _VERTEXBUF_H_

#include <celutil/basictypes.h>
#include <celutil/color.h>
#include <celmath/vecmath.h>
#include <celmath/aabox.h>


class VertexBuffer
{
 public:
    enum {
        VertexNormal   = 0x01,
        VertexColor    = 0x02,
        VertexColor0   = 0x02,
        VertexColor1   = 0x04,
        TexCoord0      = 0x08,
        TexCoord1      = 0x10,
    };

    class Vertex
    {
    public:
        Point3f point;
        Vec3f normal;
        Color color;
        Point2f texCoords[2];
    };

    union VertexPart
    {
        float f;
        unsigned char c[4];
    };

 public:
    VertexList(uint32 _parts, uint32 initialVertexPoolSize = 0);
    ~VertexList();

    void addVertex(const Vertex& v);

    Color getDiffuseColor() const;
    void setDiffuseColor(Color);

    void render();

    AxisAlignedBox getBoundingBox() const;
    void transform(Vec3f translation, float scale);

 private:
    uint32 parts;
    uint32 vertexSize;

    uint32 nVertices;
    uint32 maxVertices;
    VertexPart* vertices;

    Color diffuseColor;

    AxisAlignedBox bbox;
};

#endif // _VERTEXBUF_H_
