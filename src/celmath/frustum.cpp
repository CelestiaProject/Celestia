// frustum.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <type_traits>
#include <utility>

#include <Eigen/LU>

#include <celutil/array_view.h>
#include "frustum.h"

namespace celestia::math
{

namespace
{

constexpr auto NearIdx = static_cast<std::size_t>(FrustumPlane::Near);
constexpr auto FarIdx = static_cast<std::size_t>(FrustumPlane::Far);

void
init(Frustum::PlaneType* planes, float l, float r, float t, float b, float n)
{
    std::array<Eigen::Vector3f, 4> normals
    {
        Eigen::Vector3f(0.0f,    n,  b), // Bottom
        Eigen::Vector3f(0.0f,   -n, -t), // Top
        Eigen::Vector3f(   n, 0.0f,  l), // Left
        Eigen::Vector3f(  -n, 0.0f, -r), // Right
    };

    for (unsigned int i = 0; i < 4; i++)
    {
        planes[i] = Eigen::Hyperplane<float, 3>(normals[i].normalized(), 0.0f);
    }

    planes[NearIdx] = Eigen::Hyperplane<float, 3>(Eigen::Vector3f(0.0f, 0.0f, -1.0f), -n);
}

void
init(Frustum::PlaneType* planes, float fov, float aspectRatio, float n)
{
    float h = std::tan(fov * 0.5f);
    float w = h * aspectRatio;

    init(planes, -w * n, w * n, h * n, -h * n, n);
}

Frustum::PlaneType
createFarPlane(float f)
{
    return Frustum::PlaneType(Eigen::Vector3f(0.0f, 0.0f,  1.0f), f);
}

void
doTransform(Frustum::PlaneType* planes, unsigned int nPlanes, const Eigen::Matrix3f& matrix)
{
    for (unsigned int i = 0; i < nPlanes; ++i)
    {
        planes[i] = planes[i].transform(matrix, Eigen::Isometry);
    }
}

void
doTransform(Frustum::PlaneType* planes, unsigned int nPlanes, const Eigen::Matrix4f& matrix)
{
    Eigen::Matrix4f invTranspose = matrix.inverse().transpose();

    for (unsigned int i = 0; i < nPlanes; ++i)
    {
        planes[i].coeffs() = invTranspose * planes[i].coeffs();
        planes[i].normalize();
    }
}

FrustumAspect
doTest(util::array_view<Frustum::PlaneType> planes, const Eigen::Vector3f& point)
{
    for (const auto& plane : planes)
    {
        if (plane.signedDistance(point) < 0.0f)
            return FrustumAspect::Outside;
    }

    return FrustumAspect::Inside;
}

template<typename PREC>
FrustumAspect
doTestSphere(util::array_view<Frustum::PlaneType> planes,
             const Eigen::Matrix<PREC, 3, 1>& center,
             PREC radius)
{
    bool isIntersecting = false;

    for (const auto& plane : planes)
    {
        PREC distanceToPlane;
        if constexpr (std::is_same_v<PREC, float>)
        {
            distanceToPlane = plane.signedDistance(center);
        }
        else
        {
            // IMPORTANT: Celestia relies on this calculation being peformed at double
            // precision. Simply converting center to single precision is NOT an
            // allowable optimization.
            distanceToPlane = plane.template cast<PREC>().signedDistance(center);
        }

        if (distanceToPlane < -radius)
            return FrustumAspect::Outside;
        if (distanceToPlane <= radius)
            isIntersecting = true;
    }

    return isIntersecting ? FrustumAspect::Intersect : FrustumAspect::Inside;
}

} // end unnamed namespace

Frustum::Frustum(float fov, float aspectRatio, float n, float f)
{
    init(planes.data(), fov, aspectRatio, n);
    planes[FarIdx] = createFarPlane(f);
}

Frustum::Frustum(float l, float r, float t, float b, float n, float f)
{
    init(planes.data(), l, r, t, b, n);
    planes[FarIdx] = createFarPlane(f);
}

void
Frustum::transform(const Eigen::Matrix3f& m)
{
    doTransform(planes.data(), nPlanes, m);
}

void
Frustum::transform(const Eigen::Matrix4f& m)
{
    doTransform(planes.data(), nPlanes, m);
}

FrustumAspect
Frustum::test(const Eigen::Vector3f& point) const
{
    return doTest(planes, point);
}

FrustumAspect
Frustum::testSphere(const Eigen::Vector3f& center, float radius) const
{
    return doTestSphere(planes, center, radius);
}

/** Double precision version of testSphere()
  */
FrustumAspect
Frustum::testSphere(const Eigen::Vector3d& center, double radius) const
{
    return doTestSphere(planes, center, radius);
}

InfiniteFrustum::InfiniteFrustum(float fov, float aspectRatio, float n)
{
    init(planes.data(), fov, aspectRatio, n);
}

void
InfiniteFrustum::transform(const Eigen::Matrix3f& m)
{
    doTransform(planes.data(), nPlanes, m);
}

void
InfiniteFrustum::transform(const Eigen::Matrix4f& m)
{
    doTransform(planes.data(), nPlanes, m);
}

FrustumAspect
InfiniteFrustum::test(const Eigen::Vector3f& point) const
{
    return doTest(planes, point);
}

FrustumAspect
InfiniteFrustum::testSphere(const Eigen::Vector3f& center, float radius) const
{
    return doTestSphere(planes, center, radius);
}

/** Double precision version of testSphere()
  */
FrustumAspect
InfiniteFrustum::testSphere(const Eigen::Vector3d& center, double radius) const
{
    return doTestSphere(planes, center, radius);
}

}
