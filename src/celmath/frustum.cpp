// frustum.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "frustum.h"
#include <Eigen/LU>
#include <cmath>

using namespace Eigen;


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
    float h = std::tan(fov / 2.0f);
    float w = h * aspectRatio;

    Vector3f normals[4];
    normals[Bottom] = Vector3f( 0.0f,  1.0f, -h);
    normals[Top]    = Vector3f( 0.0f, -1.0f, -h);
    normals[Left]   = Vector3f( 1.0f,  0.0f, -w);
    normals[Right]  = Vector3f(-1.0f,  0.0f, -w);
    for (unsigned int i = 0; i < 4; i++)
    {
        planes[i] = Hyperplane<float, 3>(normals[i].normalized(), 0.0f);
    }

    planes[Near] = Hyperplane<float, 3>(Vector3f(0.0f, 0.0f, -1.0f), -n);
    planes[Far]  = Hyperplane<float, 3>(Vector3f(0.0f, 0.0f,  1.0f),  f);
}


Frustum::Aspect
Frustum::test(const Eigen::Vector3f& point) const
{
    unsigned int nPlanes = infinite ? 5 : 6;

    for (unsigned int i = 0; i < nPlanes; i++)
    {
        if (planes[i].signedDistance(point) < 0.0f)
            return Outside;
    }

    return Inside;
}


Frustum::Aspect
Frustum::testSphere(const Eigen::Vector3f& center, float radius) const
{
    unsigned int nPlanes = infinite ? 5 : 6;
    unsigned int intersections = 0;

    for (unsigned int i = 0; i < nPlanes; i++)
    {
        float distanceToPlane = planes[i].signedDistance(center);
        if (distanceToPlane < -radius)
            return Outside;
        else if (distanceToPlane <= radius)
            intersections |= (1 << i);
    }

    return (intersections == 0) ? Inside : Intersect;
}


/** Double precision version of testSphere()
  */
Frustum::Aspect
Frustum::testSphere(const Eigen::Vector3d& center, double radius) const
{
    int nPlanes = infinite ? 5 : 6;
    int intersections = 0;

    // IMPORTANT: Celestia relies on this calculation being peformed at double
    // precision. Simply converting center to single precision is NOT an
    // allowable optimization.
    for (int i = 0; i < nPlanes; i++)
    {
        double distanceToPlane = planes[i].cast<double>().signedDistance(center);
        if (distanceToPlane < -radius)
            return Outside;
        else if (distanceToPlane <= radius)
            intersections |= (1 << i);
    }

    return (intersections == 0) ? Inside : Intersect;
}


Frustum::Aspect Frustum::test(const Point3f& p) const
{
    return testSphere(Eigen::Vector3f(p.x, p.y, p.z), 0.0f);
}


Frustum::Aspect Frustum::testSphere(const Point3f& center, float radius) const
{
    return testSphere(Eigen::Vector3f(center.x, center.y, center.z), radius);
}


Frustum::Aspect Frustum::testSphere(const Point3d& center, double radius) const
{
    return testSphere(Eigen::Vector3d(center.x, center.y, center.z), radius);
}


Frustum::Aspect Frustum::testCapsule(const Capsulef& capsule) const
{
    int nPlanes = infinite ? 5 : 6;
    int intersections = 0;
    float r2 = capsule.radius * capsule.radius;
    
    // TODO: Unnecessary after Eigen conversion of Capsule class
    Vector3f capsuleOrigin(capsule.origin.x, capsule.origin.y, capsule.origin.z);
    Vector3f capsuleAxis(capsule.axis.x, capsule.axis.y, capsule.axis.z);

    for (int i = 0; i < nPlanes; i++)
    {
        float signedDist0 = planes[i].signedDistance(capsuleOrigin);
        float signedDist1 = planes[i].signedDistance(capsuleOrigin + capsuleAxis);
        //float signedDist1 = signedDist0 + planes[i].normal * capsule.axis;
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
    }

    return (intersections == 0) ? Inside : Intersect;
}


void
Frustum::transform(const Matrix3f& m)
{
    unsigned int nPlanes = infinite ? 5 : 6;

    for (unsigned int i = 0; i < nPlanes; i++)
    {
        planes[i] = planes[i].transform(m, Eigen::Isometry);
    }
}


void
Frustum::transform(const Matrix4f& m)
{
    unsigned int nPlanes = infinite ? 5 : 6;
    Matrix4f invTranspose = m.inverse().transpose();

    for (unsigned int i = 0; i < nPlanes; i++)
    {
        planes[i].coeffs() = invTranspose * planes[i].coeffs();
        planes[i].normalize();

        //float s = 1.0f / planes[i].normal().norm();
        //planes[i].normal() *= s;
        //planes[i].offset() *= s;

        //planes[i] = planes[i] * invTranspose;
        //float s = 1.0f / planes[i].normal().norm();
        //planes[i].normal = planes[i].normal * s;
        //planes[i].d *= s;
    }
}


void
Frustum::transform(const Mat3f& m)
{
    Matrix3f m2 = Map<Matrix3f>(&m[0][0]);
    transform(m2);
}


void
Frustum::transform(const Mat4f& m)
{
    Matrix4f m2 = Map<Matrix4f>(&m[0][0]);
    transform(m2);
}
