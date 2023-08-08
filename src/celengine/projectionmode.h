// projectionmode.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <optional>

class ShaderManager;

namespace celmath
{
class Frustum;
}

namespace celestia::engine
{

constexpr float standardFOV = 45.0f;

class ProjectionMode
{
public:
    ProjectionMode(float width, float height, int distantToScreen, int screenDpi);
    virtual ~ProjectionMode() = default;

    virtual Eigen::Matrix4f getProjectionMatrix(float nearZ, float farZ, float zoom) const = 0;
    virtual float getMinimumFOV() const = 0;
    virtual float getMaximumFOV() const = 0;
    virtual float getFOV(float zoom) const = 0;
    virtual float getZoom(float fov) const = 0;
    virtual float getPixelSize(float zoom) const = 0;
    virtual float getFieldCorrection(float zoom) const = 0;
    virtual celmath::Frustum getFrustum(float nearZ, std::optional<float> farZ, float zoom) const = 0;

    // Calculate the cosine of half the maximum field of view. We'll use this for
    // fast testing of object visibility.
    virtual double getViewConeAngleMax(float zoom) const = 0;

    virtual float getNormalizedDeviceZ(float nearZ, float farZ, float z) const = 0;

    virtual Eigen::Vector3f getPickRay(float x, float y, float zoom) const = 0;
    virtual void configureShaderManager(ShaderManager *) const = 0;
    virtual bool project(const Eigen::Vector3f& pos, const Eigen::Matrix4f existingModelViewMatrix, const Eigen::Matrix4f existingProjectionMatrix, const Eigen::Matrix4f existingMVPMatrix, const int viewport[4], Eigen::Vector3f& result) const = 0;

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
