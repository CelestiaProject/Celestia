// trilist.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// A very simple triangle list class for quickie OpenGL programs
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "gl.h"
#include "trilist.h"


TriangleList::TriangleList() :
    nTriangles(0),
    maxTriangles(0),
    vertices(NULL),
    normals(NULL),
    color(1, 1, 1),
    colorMode(0),
    bboxValid(true)
{
}

TriangleList::~TriangleList()
{
    if (vertices != NULL)
        delete[] vertices;
    if (normals != NULL)
        delete[] normals;
}

void TriangleList::clear()
{
    nTriangles = 0;
}

void TriangleList::render()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glNormalPointer(GL_FLOAT, 0, normals);
    if (colorMode == 1)
        glColor4f(color.x, color.y, color.z, 1);
    glDrawArrays(GL_TRIANGLES, 0, nTriangles * 3);
}


// Add a triangle with vertex normals
void TriangleList::addTriangle(Point3f& p0, Vec3f& n0,
                               Point3f& p1, Vec3f& n1,
                               Point3f& p2, Vec3f& n2)
{
    if (nTriangles == maxTriangles)
    {
        if (maxTriangles == 0)
        {
            maxTriangles = 16;
            vertices = new float[maxTriangles * 9];
            normals = new float[maxTriangles * 9];
        }
        else
        {
            float* newVertices = new float[maxTriangles * 2 * 9];
            memcpy((void*) newVertices,
                   (void*) vertices,
                   sizeof(float) * maxTriangles * 9);
            delete vertices;
            vertices = newVertices;

            float* newNormals = new float[maxTriangles * 2 * 9];
            memcpy((void*) newNormals,
                   (void*) normals,
                   sizeof(float) * maxTriangles * 9);
            delete normals;
            normals = newNormals;

            maxTriangles *= 2;
        }
    }

    int n = nTriangles * 9;
    vertices[n    ] = p0.x;
    vertices[n + 1] = p0.y;
    vertices[n + 2] = p0.z;
    vertices[n + 3] = p1.x;
    vertices[n + 4] = p1.y;
    vertices[n + 5] = p1.z;
    vertices[n + 6] = p2.x;
    vertices[n + 7] = p2.y;
    vertices[n + 8] = p2.z;
    normals[n    ] = n0.x;
    normals[n + 1] = n0.y;
    normals[n + 2] = n0.z;
    normals[n + 3] = n1.x;
    normals[n + 4] = n1.y;
    normals[n + 5] = n1.z;
    normals[n + 6] = n2.x;
    normals[n + 7] = n2.y;
    normals[n + 8] = n2.z;

    nTriangles++;
    bboxValid = false;
}

// Add a triangle with the specified face normal
void TriangleList::addTriangle(Point3f& p0, Point3f& p1, Point3f& p2,
                               Vec3f& normal)
{
    addTriangle(p0, normal, p1, normal, p2, normal);
}

// Add a triangle, normal is computed automatically
void TriangleList::addTriangle(Point3f& p0, Point3f& p1, Point3f& p2)
{
    // Compute the normal
    Vec3f dv0 = p1 - p0;
    Vec3f dv1 = p2 - p1;
    Vec3f normal = cross(dv0, dv1);
    normal.normalize();

    addTriangle(p0, p1, p2, normal);
}


int TriangleList::triangleCount()
{
    return nTriangles;
}

Vec3f TriangleList::getColor() const
{
    return color;
}

void TriangleList::setColor(Vec3f _color)
{
    color = _color;
}

void TriangleList::setColorMode(int _colorMode)
{
    colorMode = _colorMode;
}

// Apply a translation and uniform scale to the vertices
void TriangleList::transform(Vec3f move, float scale)
{
    printf("TriangleList::transform : scale = %f\n", scale);
    for (int i = 0; i < nTriangles * 3; i++)
    {
        int n = i * 3;
        vertices[n    ] = (vertices[n    ] - move.x) * scale;
        vertices[n + 1] = (vertices[n + 1] - move.y) * scale;
        vertices[n + 2] = (vertices[n + 2] - move.z) * scale;
    }
}


AxisAlignedBox TriangleList::getBoundingBox()
{
    if (!bboxValid)
    {
        bbox = AxisAlignedBox();
        for (int i = 0; i < nTriangles * 9; i += 3)
            bbox.include(Point3f(vertices[i], vertices[i + 1], vertices[i + 2]));
    }

    return bbox;
}
