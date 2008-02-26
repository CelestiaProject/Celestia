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
    planes[Far] = Planef(Vec3f(0, 0, 1), f);
}


Frustum::Aspect Frustum::test(const Point3f& p) const
{
    return testSphere(p, 0);
}


Frustum::Aspect Frustum::testSphere(const Point3f& center, float radius) const
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


Frustum::Aspect Frustum::testSphere(const Point3d& center, double radius) const
{
    int nPlanes = infinite ? 5 : 6;
    int intersections = 0;

    for (int i = 0; i < nPlanes; i++)
    {
        //TODO: Celestia should incorporate some casting operators overloading to accommodate all this kind of stuff:
        Vec3f plNormal       = planes[i].normal;
        Vec3d plNormalDbl(plNormal.x, plNormal.y, plNormal.z); 
         
        double distanceToPlane = plNormalDbl.x * center.x + plNormalDbl.y * center.y + plNormalDbl.z * center.z + planes[i].d;        
        if (distanceToPlane < -radius)
            return Outside;
        else if (distanceToPlane <= radius)
            intersections |= (1 << i);
    }

    return (intersections == 0) ? Inside : Intersect;
}


Frustum::Aspect Frustum::testCapsule(const Capsulef& capsule) const
{
    int nPlanes = infinite ? 5 : 6;
    int intersections = 0;
    float r2 = capsule.radius * capsule.radius;
    
    for (int i = 0; i < nPlanes; i++)
    {
        float signedDist0 = planes[i].normal * Vec3f(capsule.origin.x, capsule.origin.y, capsule.origin.z) + planes[i].d;
        float signedDist1 = signedDist0 + planes[i].normal * capsule.axis;
        if (signedDist0 * signedDist1 > r2)
        {
            // Endpoints of capsule are on same side of plane; test closest endpoint to see if it
            // lies closer to the plane than radius
            if (abs(signedDist0) <= abs(signedDist1))
            {
                if (signedDist0 < -capsule.radius)
                    return Outside;
                else if (signedDist0 < capsule.radius)
                    intersections |= (1 << i);
            }
            else
            {
                if (signedDist1 < -capsule.radius)
                    return Outside;
                else if (signedDist1 < capsule.radius)
                    intersections |= (1 << i);
            }
        }
        else
        {
            // Capsule endpoints are on different sides of the plane, so we have an intersection
            intersections |= (1 << i);
        }
#if 0        
        Vec3f plNormal       = planes[i].normal;
        Vec3d plNormalDbl(plNormal.x, plNormal.y, plNormal.z); 
         
        double distanceToPlane = planes[i].distanceToSegment(capsule.origin, capsule.axis);
        if (distanceToPlane < -capsule.radius)
            return Outside;
        else if (distanceToPlane <= capsule.radius)
            intersections |= (1 << i);
#endif
    }

    return (intersections == 0) ? Inside : Intersect;
}


#if 0
// See if the half space defined by the plane intersects the frustum.  For the
// plane equation p(x, y, z) = ax + by + cz + d = 0, the halfspace is
// contains all points p(x, y, z) < 0.
Frustum::Aspect Frustum::testHalfSpace(const Planef& plane)
{
    return Intersect;
}
#endif


void Frustum::transform(const Mat3f& m)
{
    int nPlanes = infinite ? 5 : 6;
    Mat3f invTranspose = m.inverse().transpose();

    for (int i = 0; i < nPlanes; i++)
    {
        planes[i] = planes[i] * invTranspose;
        float s = 1.0f / planes[i].normal.length();
        planes[i].normal = planes[i].normal * s;
        planes[i].d *= s;
    }
}


void Frustum::transform(const Mat4f& m)
{
    int nPlanes = infinite ? 5 : 6;
    Mat4f invTranspose = m.inverse().transpose();

    for (int i = 0; i < nPlanes; i++)
    {
        planes[i] = planes[i] * invTranspose;
        float s = 1.0f / planes[i].normal.length();
        planes[i].normal = planes[i].normal * s;
        planes[i].d *= s;
    }
}
