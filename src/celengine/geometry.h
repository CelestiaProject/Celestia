// geometry.h
//
// Copyright (C) 2004-present, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <Eigen/Geometry>

#include <celmodel/material.h>

class RenderContext;

class RenderGeometry
{
public:
    virtual ~RenderGeometry() = default;

    virtual void render(RenderContext& rc, double t = 0.0) = 0;

    virtual bool isOpaque() const { return true; }
    virtual bool isNormalized() const { return true; }
};

class Geometry
{
public:
    virtual ~Geometry() = default;

    virtual std::unique_ptr<RenderGeometry> createRenderGeometry() const = 0;

    /*! Find the closest intersection between the ray and the
     *  model.  If the ray intersects the model, return true
     *  and set distance; otherwise return false and leave
     *  distance unmodified.
     */
    virtual bool pick(const Eigen::ParametrizedLine<double, 3>& r, double& distance) const = 0;

    virtual bool isNormalized() const { return true; }
};

class EmptyGeometry : public Geometry
{
    std::unique_ptr<RenderGeometry> createRenderGeometry() const override;
    bool pick(const Eigen::ParametrizedLine<double, 3>&, double&) const override { return false; }
};
