// frustum.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "frustum.h"


Frustum::Frustum(float fov, float aspectRatio, float n) :
    infinite(true)
{
    init(fov, aspectRatio, n, n);
}


Frustum::Frustum(float fov, float aspectRatio, float n, float f) :
    infinite(false)
{
    init(fov, aspectRatio, n, f);
}


void Frustum::init(float fov, float aspectRatio, float n, float f)
{
    float h = (float) tan(fov / 2.0f);
    float w = h * aspectRatio;

    Vec3f normals[4];
    normals[Bottom] = Vec3f(0, 1, -h);
    normals[Top]    = Vec3f(0, -1, -h);
    normals[Left]   = Vec3f(1, 0, -w);
    normals[Right]  = Vec3f(-1, 0, -w);
    for (int i = 0; i < 4; i++)
    {
        normals[i].normalize();
        planes[i] = Planef(normals[i], Point3f(0, 0, 0));
    }

    planes[Near] = Planef(Vec3f(0, 0, -1), -n);
    planes[Far] = Planef(Vec3f(0, 0,  1), -f);
}


Frustum::Aspect Frustum::test(const Point3f& p)
{
    return testSphere(p, 0);
}


Frustum::Aspect Frustum::testSphere(const Point3f& center, float radius)
{
    int nPlanes = infinite ? 5 : 6;
    int intersections = 0;

    for (int i = 0; i < nPlanes; i++)
    {
        float distanceToPlane = planes[i].distanceTo(center);
        if (distanceToPlane < -radius)
            return Outside;
        else if (distanceToPlane <= radius)
            intersections |= (1 << i);
    }

    return (intersections == 0) ? Inside : Intersect;
}


void Frustum::transform(const Mat3f&)
{
}


void Frustum::transform(const Mat4f&)
{
}
