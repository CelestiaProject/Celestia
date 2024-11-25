// ringrenderer.h
//
// Copyright (C) 2006-2024, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "gl/buffer.h"
#include "gl/vertexobject.h"

class LightingState;
struct Matrices;
class Renderer;
struct RenderInfo;
struct RingSystem;

namespace celestia::render
{

class RingRenderer
{
public:
    explicit RingRenderer(Renderer&);
    ~RingRenderer() = default;

    void renderRings(RingSystem& rings,
                     const RenderInfo& ri,
                     const LightingState& ls,
                     float planetRadius,
                     float planetOblateness,
                     bool renderShadow,
                     float segmentSizeInPixels,
                     const Matrices &m,
                     bool inside);

private:
    void initializeLOD(unsigned int, std::uint32_t);
    void renderLOD(unsigned int, std::uint32_t);

    static constexpr unsigned int nLODs = 4;

    std::array<float, nLODs - 1> sectionScales;
    std::array<std::optional<gl::Buffer>, nLODs> buffers;
    std::array<std::optional<gl::VertexObject>, nLODs> vertexObjects;
    Renderer& renderer;
};

} // end namespace celestia::render
