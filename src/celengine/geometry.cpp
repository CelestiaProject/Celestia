// geometry.cpp
//
// Copyright (C) 2025-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "geometry.h"

namespace
{

class EmptyRenderGeometry : public RenderGeometry
{
public:
    void render(RenderContext&, double = 0.0) override { /* nothing to render */ }
};

} // end unnamed namespace

std::unique_ptr<RenderGeometry>
EmptyGeometry::createRenderGeometry() const
{
    return std::make_unique<EmptyRenderGeometry>();
}
