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

#include <celutil/reshandle.h>
#include "multitexture.h"
#include "renderflags.h"

struct Atmosphere;
class Geometry;
class LightingState;
class LODSphereMesh;
struct Matrices;
class Renderer;
struct RenderInfo;
class Texture;

namespace celestia::math
{
class Frustum;
}

void renderEllipsoid_GLSL(const RenderInfo& ri,
                          const LightingState& ls,
                          Atmosphere* atmosphere,
                          float cloudTexOffset,
                          const Eigen::Vector3f& semiAxes,
                          TextureResolution textureRes,
                          RenderFlags renderFlags,
                          const Eigen::Quaternionf& planetOrientation,
                          const celestia::math::Frustum& frustum,
                          const Matrices &m,
                          Renderer* renderer,
                          LODSphereMesh* lodSphere);

void renderGeometry_GLSL(Geometry* geometry,
                         const RenderInfo& ri,
                         ResourceHandle texOverride,
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
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       const Eigen::Vector3f& semiAxes,
                       TextureResolution textureRes,
                       RenderFlags renderFlags,
                       const Eigen::Quaternionf& planetOrientation,
                       const celestia::math::Frustum& frustum,
                       const Matrices &m,
                       Renderer* renderer,
                       LODSphereMesh* lodSphere);

void renderGeometry_GLSL_Unlit(Geometry* geometry,
                               const RenderInfo& ri,
                               ResourceHandle texOverride,
                               double tsec,
                               const Matrices &m,
                               Renderer* renderer);
