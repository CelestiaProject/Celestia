// trilist.h
//
// Copyright (C) 2001, Chris Laurel
//
// A very simple triangle list class for quickie OpenGL programs
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TRILIST_H_
#define _TRILIST_H_

#include "vecmath.h"
#include "aabox.h"

class TriangleList
{
 public:
    TriangleList();
    ~TriangleList();

    void clear();
    void addTriangle(Point3f&, Vec3f&,
                     Point3f&, Vec3f&,
                     Point3f&, Vec3f&);
    void addTriangle(Point3f&, Point3f&, Point3f&, Vec3f&);
    void addTriangle(Point3f&, Point3f&, Point3f&);
    void render();
    int triangleCount();

    Vec3f getColor() const;
    void setColor(Vec3f);
    void setColorMode(int);

    // Apply a translation and uniform scale to the vertices
    void transform(Vec3f move, float scale);

    AxisAlignedBox getBoundingBox();

 private:
    // Override the default copy constructor
    TriangleList(TriangleList& tl) {};

 private:
    int nTriangles;
    int maxTriangles;
    float *vertices;
    float *normals;

    int colorMode;
    Vec3f color;

    bool bboxValid;
    AxisAlignedBox bbox;
};

#endif // _TRILIST_H_

