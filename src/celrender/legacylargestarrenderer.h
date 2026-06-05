// legacylargestarrenderer.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celrender/largestarrenderer.h>

class Texture;

namespace celestia::render
{

// Batched textured-billboard renderer for point-sprite stars whose
// gl_PointSize would exceed the driver's GL_ALIASED_POINT_SIZE_RANGE.
class LegacyLargeStarRenderer : public LargeStarRenderer
{
public:
    explicit LegacyLargeStarRenderer(Renderer &renderer, capacity_t capacity = 512);
    ~LegacyLargeStarRenderer() override;

    void setTexture(Texture *texture) { m_texture = texture; }

protected:
    void onMakeCurrent(const Eigen::Vector2f &viewportRcp) override;

private:
    Texture *m_texture { nullptr };
};

} // namespace celestia::render
