// renderglsl.h
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celutil/texhandle.h>
#include "renderflags.h"

struct Atmosphere;
class LightingState;
class LODSphereMesh;
struct Matrices;
class Renderer;
class RenderGeometry;
struct RenderInfo;
class Texture;

namespace celestia::math
{
class Frustum;
}

void renderEllipsoid_GLSL(const RenderInfo& ri,
                          const LightingState& ls,
                          const Atmosphere* atmosphere,
                          float cloudTexOffset,
                          const Eigen::Vector3f& semiAxes,
                          RenderFlags renderFlags,
                          const Eigen::Quaternionf& planetOrientation,
                          const celestia::math::Frustum& frustum,
                          const Matrices &m,
                          Renderer* renderer,
                          LODSphereMesh* lodSphere);

void renderGeometry_GLSL(RenderGeometry* geometry,
                         const RenderInfo& ri,
                         celestia::util::TextureHandle texOverride,
                         const LightingState& ls,
                         const Atmosphere* atmosphere,
                         float geometryScale,
                         RenderFlags renderFlags,
                         const Eigen::Quaternionf& planetOrientation,
                         double tsec,
                         const Matrices &m,
                         Renderer* renderer);

void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       const Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       const Eigen::Vector3f& semiAxes,
                       RenderFlags renderFlags,
                       const Eigen::Quaternionf& planetOrientation,
                       const celestia::math::Frustum& frustum,
                       const Matrices &m,
                       Renderer* renderer,
                       LODSphereMesh* lodSphere);

void renderGeometry_GLSL_Unlit(RenderGeometry* geometry,
                               const RenderInfo& ri,
                               celestia::util::TextureHandle texOverride,
                               double tsec,
                               const Matrices &m,
                               Renderer* renderer);
