// perspectiveprojectionmode.cpp
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "perspectiveprojectionmode.h"
#include <celmath/frustum.h>
#include <celmath/geomutil.h>
#include <celengine/shadermanager.h>

namespace celestia::engine
{

PerspectiveProjectionMode::PerspectiveProjectionMode(float width, float height, int distanceToScreen, int screenDpi) :
    ProjectionMode(width, height, distanceToScreen, screenDpi)
{
}

Eigen::Matrix4f PerspectiveProjectionMode::getProjectionMatrix(float nearZ, float farZ, float zoom) const
{
    return math::Perspective(math::radToDeg(getFOV(zoom)), width / height, nearZ, farZ);
}

float PerspectiveProjectionMode::getMinimumFOV() const
{
    return math::degToRad(0.001f);
}

float PerspectiveProjectionMode::getMaximumFOV() const
{
    return math::degToRad(120.0f);
}

float PerspectiveProjectionMode::getFOV(float zoom) const
{
    return math::PerspectiveFOV(height, screenDpi, distanceToScreen) / zoom;
}

float PerspectiveProjectionMode::getZoom(float fov) const
{
    return math::PerspectiveFOV(height, screenDpi, distanceToScreen) / fov;
}

float PerspectiveProjectionMode::getPixelSize(float zoom) const
{
    return 2.0f * std::tan(getFOV(zoom) * 0.5f) /  height;
}

float PerspectiveProjectionMode::getFieldCorrection(float zoom) const
{
    return 2.0f * standardFOV / (math::radToDeg(getFOV(zoom)) + standardFOV);
}

math::Frustum
PerspectiveProjectionMode::getFrustum(float nearZ, float farZ, float zoom) const
{
    return math::Frustum(getFOV(zoom), width / height, nearZ, farZ);
}

math::InfiniteFrustum
PerspectiveProjectionMode::getInfiniteFrustum(float nearZ, float zoom) const
{
    return math::InfiniteFrustum(getFOV(zoom), width / height, nearZ);
}

double PerspectiveProjectionMode::getViewConeAngleMax(float zoom) const
{
    // When computing the view cone, we want the field of
    // view as measured on the diagonal between viewport corners.
    double h = std::tan(static_cast<double>(getFOV(zoom)) / 2.0);
    double w = h * static_cast<double>(width) / static_cast<double>(height);
    double diag = std::sqrt(1.0 + h * h + w * w);
    return 1.0 / diag;
}

float PerspectiveProjectionMode::getNormalizedDeviceZ(float nearZ, float farZ, float z) const
{
    float d0 = farZ - nearZ;
    float d1 = -(farZ + nearZ) / d0;
    float d2 = -2.0f * nearZ * farZ / d0;
    return d1 - d2 / z;
}

Eigen::Vector3f PerspectiveProjectionMode::getPickRay(float x, float y, float zoom) const
{
    float s = 2.0f * std::tan(getFOV(zoom) / 2.0f);
    Eigen::Vector3f pickDirection(x * s, y * s, -1.0f);

    return pickDirection.normalized();
}

void PerspectiveProjectionMode::configureShaderManager(ShaderManager *shaderManager) const
{
    shaderManager->setFisheyeEnabled(false);
}

bool PerspectiveProjectionMode::project(const Eigen::Vector3f& pos,
                                        const Eigen::Matrix4f& /*existingModelViewMatrix*/,
                                        const Eigen::Matrix4f& /*existingProjectionMatrix*/,
                                        const Eigen::Matrix4f& existingMVPMatrix,
                                        const std::array<int, 4>& viewport,
                                        Eigen::Vector3f& result) const
{
    return math::ProjectPerspective(pos, existingMVPMatrix, viewport, result);
}

}
