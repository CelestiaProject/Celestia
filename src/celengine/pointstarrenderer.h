// pointstarrenderer.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>

#include <Eigen/Core>

#include <celutil/color.h>

#include "objectrenderer.h"
#include "renderflags.h"
#include "renderlistentry.h"

class ColorTemperatureTable;
class PointStarVertexBuffer;
namespace celestia::render { class PsfGlowLargeRenderer; }
namespace celestia::render { class PsfStarVertexBuffer; }
class Star;
class StarDatabase;

// TODO: move these variables to PointStarRenderer class
// without adding a variable. Requires C++17
constexpr inline float StarDistanceLimit     = 1.0e6f;
// Star disc size in pixels
constexpr inline float BaseStarDiscSize      = 5.0f;
constexpr inline float MaxScaledDiscStarSize = 8.0f;
constexpr inline float GlareOpacity          = 0.65f;

class PointStarRenderer : public ObjectRenderer<Star, float>
{
public:
    // PSF-specific configuration and per-frame state, grouped to keep
    // PointStarRenderer's top-level field count manageable.
    struct PsfState
    {
        celestia::render::PsfStarVertexBuffer*  pointBuffer       { nullptr };
        celestia::render::PsfStarVertexBuffer*  glowBuffer        { nullptr };
        celestia::render::PsfGlowLargeRenderer* glowLargeRenderer { nullptr };

        // Projection/modelview used by the large-glow fallback for
        // far-star PSF blooms whose gl_PointSize would exceed the
        // driver's maximum.  Same matrices the psf glow buffer flushes
        // against.
        const Eigen::Matrix4f* proj      { nullptr };
        const Eigen::Matrix4f* modelView { nullptr };

        // User-facing parameters (set per-frame from Renderer state).
        float pointRadius    { 1.5f };   // px
        float pointScale     { 1.0f };   // DPI scale
        float optimization   { 0.1f };
        float maxIrradiance  { 0.0f };   // 0 = disabled
        float exposure       { 1.0f };

        // Per-frame derived constants, hoisted out of process() to keep
        // the per-star inner loop tight.  Filled in renderPointStars
        // before findVisibleStars runs.
        float peakRadScale          { 0.0f }; // exposure * 3 / (pi * r^2)
        float dimGate               { 0.0f }; // soft-clip threshold; peak <= dimGate culled
        float glowA                 { 0.0f }; // optimization / r
        float glowPeakLargeThreshold{ 0.0f }; // glowPeak above which sizePhys > maxPointSize
    };

    PointStarRenderer();
    void process(const Star &star, float distance, float appMag) override;

    Eigen::Vector3d obsPos;
    Eigen::Vector3f viewNormal;
    std::vector<RenderListEntry>* renderList    { nullptr };
    PointStarVertexBuffer* starVertexBuffer     { nullptr };
    PointStarVertexBuffer* glareVertexBuffer    { nullptr };
    const StarDatabase* starDB                  { nullptr };
    const ColorTemperatureTable* colorTemp      { nullptr };
    float SolarSystemMaxDistance                { 1.0f };
    float cosFOV                                { 1.0f };

    StarStyle starStyle                         { StarStyle::FuzzyPointStars };
    PsfState  psf;
};
