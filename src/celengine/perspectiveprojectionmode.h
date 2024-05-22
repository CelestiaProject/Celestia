// perspectiveprojectionmode.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

#include <celengine/projectionmode.h>

namespace celestia::engine
{

class PerspectiveProjectionMode : public ProjectionMode
{
public:
    PerspectiveProjectionMode(float width, float height, int distanceToScreen, int screenDpi);

    PerspectiveProjectionMode(const PerspectiveProjectionMode &) = default;
    PerspectiveProjectionMode(PerspectiveProjectionMode &&) = default;
    PerspectiveProjectionMode &operator=(const PerspectiveProjectionMode &) = default;
    PerspectiveProjectionMode &operator=(PerspectiveProjectionMode &&) = default;
    ~PerspectiveProjectionMode() override = default;

    Eigen::Matrix4f getProjectionMatrix(float nearZ, float farZ, float zoom) const override;
    float getMinimumFOV() const override;
    float getMaximumFOV() const override;
    float getFOV(float zoom) const override;
    float getZoom(float fov) const override;
    float getPixelSize(float zoom) const override;
    float getFieldCorrection(float zoom) const override;
    math::Frustum getFrustum(float nearZ, float farZ, float zoom) const override;
    math::InfiniteFrustum getInfiniteFrustum(float nearZ, float zoom) const override;
    double getViewConeAngleMax(float zoom) const override;

    float getNormalizedDeviceZ(float nearZ, float farZ, float z) const override;

    Eigen::Vector3f getPickRay(float x, float y, float zoom) const override;

    void configureShaderManager(ShaderManager *) const override;
    bool project(const Eigen::Vector3f& pos,
                 const Eigen::Matrix4f& existingModelViewMatrix,
                 const Eigen::Matrix4f& existingProjectionMatrix,
                 const Eigen::Matrix4f& existingMVPMatrix,
                 const std::array<int, 4>& viewport,
                 Eigen::Vector3f& result) const override;
};

} // end namespace celestia::engine
