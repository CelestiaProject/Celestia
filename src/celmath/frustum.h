// frustum.h
//
// Copyright (C) 2000-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstddef>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace celestia::math
{

enum class FrustumPlane : unsigned int
{
    Bottom    = 0,
    Top       = 1,
    Left      = 2,
    Right     = 3,
    Near      = 4,
    Far       = 5,
};

enum class FrustumAspect
{
    Outside   = 0,
    Inside    = 1,
    Intersect = 2,
};

class Frustum
{
public:
    using PlaneType = Eigen::Hyperplane<float, 3>;

    Frustum(float fov, float aspectRatio, float nearDist, float farDist);
    Frustum(float left, float right, float top, float bottom, float nearDist, float farDist);

    inline PlaneType plane(FrustumPlane which) const
    {
        return planes[static_cast<std::size_t>(which)];
    }

    void transform(const Eigen::Matrix3f& m);
    void transform(const Eigen::Matrix4f& m);

    FrustumAspect test(const Eigen::Vector3f& point) const;
    FrustumAspect testSphere(const Eigen::Vector3f& center, float radius) const;
    FrustumAspect testSphere(const Eigen::Vector3d& center, double radius) const;

private:
    static constexpr unsigned int nPlanes = 6;
    std::array<PlaneType, nPlanes> planes;
};

class InfiniteFrustum
{
public:
    using PlaneType = Frustum::PlaneType;

    InfiniteFrustum(float fov, float aspectRatio, float nearDist);

    inline PlaneType plane(FrustumPlane which) const
    {
        return planes[static_cast<std::size_t>(which)];
    }

    void transform(const Eigen::Matrix3f& m);
    void transform(const Eigen::Matrix4f& m);

    FrustumAspect test(const Eigen::Vector3f& point) const;
    FrustumAspect testSphere(const Eigen::Vector3f& center, float radius) const;
    FrustumAspect testSphere(const Eigen::Vector3d& center, double radius) const;

private:
    static constexpr unsigned int nPlanes = 5;
    std::array<PlaneType, nPlanes> planes;
};

} // namespace celestia::math
