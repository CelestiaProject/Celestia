// projectionmode.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <tuple>

#include <Eigen/Core>

class ShaderManager;

namespace celestia::math
{
class Frustum;
class InfiniteFrustum;
}

namespace celestia::engine
{

constexpr inline float standardFOV = 45.0f;

class ProjectionMode
{
public:
    ProjectionMode(float width, float height, int distantToScreen, int screenDpi);
    virtual ~ProjectionMode() = default;

    virtual Eigen::Matrix4f getProjectionMatrix(float nearZ, float farZ, float zoom) const = 0;
    // Some rendering systems provide nearZ, farZ. We'll use this value to set up
    // the basic projection matrix for rendering.
    virtual std::tuple<float, float> getDefaultDepthRange() const;
    virtual float getMinimumFOV() const = 0;
    virtual float getMaximumFOV() const = 0;
    virtual float getFOV(float zoom) const = 0;
    virtual float getZoom(float fov) const = 0;
    virtual float getPixelSize(float zoom) const = 0;
    virtual float getFieldCorrection(float zoom) const = 0;
    virtual celestia::math::Frustum getFrustum(float nearZ, float farZ, float zoom) const = 0;
    virtual celestia::math::InfiniteFrustum getInfiniteFrustum(float nearZ, float zoom) const = 0;

    // Calculate the cosine of half the maximum field of view. We'll use this for
    // fast testing of object visibility.
    virtual double getViewConeAngleMax(float zoom) const = 0;

    virtual float getNormalizedDeviceZ(float nearZ, float farZ, float z) const = 0;

    virtual Eigen::Vector3f getPickRay(float x, float y, float zoom) const = 0;
    virtual void configureShaderManager(ShaderManager *) const = 0;
    virtual bool project(const Eigen::Vector3f& pos,
                         const Eigen::Matrix4f& existingModelViewMatrix,
                         const Eigen::Matrix4f& existingProjectionMatrix,
                         const Eigen::Matrix4f& existingMVPMatrix,
                         const std::array<int, 4>& viewport,
                         Eigen::Vector3f& result) const = 0;

    void setScreenDpi(int screenDpi);
    void setDistanceToScreen(int distanceToScreen);
    void setSize(float width, float height);

protected:
    float width;
    float height;
    int distanceToScreen;
    int screenDpi;
};

}
