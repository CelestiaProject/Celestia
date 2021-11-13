// frustum.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>

#include <Eigen/LU>

#include "frustum.h"


namespace celmath
{

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

    Eigen::Vector3f normals[4];
    normals[Bottom] = Eigen::Vector3f( 0.0f,  1.0f, -h);
    normals[Top]    = Eigen::Vector3f( 0.0f, -1.0f, -h);
    normals[Left]   = Eigen::Vector3f( 1.0f,  0.0f, -w);
    normals[Right]  = Eigen::Vector3f(-1.0f,  0.0f, -w);
    for (unsigned int i = 0; i < 4; i++)
    {
        planes[i] = Eigen::Hyperplane<float, 3>(normals[i].normalized(), 0.0f);
    }

    planes[Near] = Eigen::Hyperplane<float, 3>(Eigen::Vector3f(0.0f, 0.0f, -1.0f), -n);
    planes[Far]  = Eigen::Hyperplane<float, 3>(Eigen::Vector3f(0.0f, 0.0f,  1.0f),  f);
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
        if (distanceToPlane <= radius)
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
        if (distanceToPlane <= radius)
            intersections |= (1 << i);
    }

    return (intersections == 0) ? Inside : Intersect;
}


void
Frustum::transform(const Eigen::Matrix3f& m)
{
    unsigned int nPlanes = infinite ? 5 : 6;

    for (unsigned int i = 0; i < nPlanes; i++)
    {
        planes[i] = planes[i].transform(m, Eigen::Isometry);
    }
}


void
Frustum::transform(const Eigen::Matrix4f& m)
{
    unsigned int nPlanes = infinite ? 5 : 6;
    Eigen::Matrix4f invTranspose = m.inverse().transpose();

    for (unsigned int i = 0; i < nPlanes; i++)
    {
        planes[i].coeffs() = invTranspose * planes[i].coeffs();
        planes[i].normalize();
    }
}

}
