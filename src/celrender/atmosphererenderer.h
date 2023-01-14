// atmosphererenderer.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Original version Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <array>
#include <vector>
#include <Eigen/Core>

class Atmosphere;
class Renderer;
class RenderInfo;
class LightingState;
struct Matrices;

namespace celmath
{
class Frustum;
}

namespace celestia::render
{
class IndexedVertexObject;

class AtmosphereRenderer
{
public:
    explicit AtmosphereRenderer(Renderer &renderer);
    ~AtmosphereRenderer() = default;
    AtmosphereRenderer(const AtmosphereRenderer&) = delete;
    AtmosphereRenderer(AtmosphereRenderer&&) = delete;
    AtmosphereRenderer& operator=(const AtmosphereRenderer&) = delete;
    AtmosphereRenderer& operator=(AtmosphereRenderer&&) = delete;

    void renderLegacy(
        const Atmosphere         &atmosphere,
        const LightingState      &ls,
        const Eigen::Vector3f    &center,
        const Eigen::Quaternionf &orientation,
        const Eigen::Vector3f    &semiAxes,
        const Eigen::Vector3f    &sunDirection,
        float                     pixSize,
        bool                      lit,
        const Matrices           &m);

    void render(
        const RenderInfo         &ri,
        const Atmosphere         &atmosphere,
        const LightingState      &ls,
        const Eigen::Quaternionf &planetOrientation,
        float                     radius,
        const celmath::Frustum   &frustum,
        const Matrices           &m);

    void initGL();
    void deinitGL();

private:
    void computeLegacy(
        const Atmosphere         &atmosphere,
        const LightingState      &ls,
        const Eigen::Vector3f    &center,
        const Eigen::Quaternionf &orientation,
        const Eigen::Vector3f    &semiAxes,
        const Eigen::Vector3f    &sunDirection,
        float                     pixSize,
        bool                      lit);

    void initVertexObject();

    struct SkyVertex
    {
        std::array<float,   3> position;
        std::array<uint8_t, 4> color;
    };

    struct SkyContourPoint
    {
        Eigen::Vector3f v;
        Eigen::Vector3f eyeDir;
        float centerDist;
        float eyeDist;
        float cosSkyCapAltitude;
    };

    Renderer                               &m_renderer;
    std::vector<SkyVertex>                  m_skyVertices;
    std::vector<unsigned short>             m_skyIndices;
    std::vector<SkyContourPoint>            m_skyContour;
    std::unique_ptr<IndexedVertexObject>    m_vo;

    bool                                    m_initialized   { false };
};
} // namespace celestia::render
