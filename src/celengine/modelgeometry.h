// modelgeometry.h
//
// Copyright (C) 2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <Eigen/Geometry>

#include <celmodel/model.h>
#include "geometry.h"

class RenderContext;

class ModelGeometry : public Geometry
{
public:
    explicit ModelGeometry(std::unique_ptr<const cmod::Model>&& model);
    ~ModelGeometry();

    std::unique_ptr<RenderGeometry> createRenderGeometry() const override;

    /*! Find the closest intersection between the ray and the
     *  model.  If the ray intersects the model, return true
     *  and set distance; otherwise return false and leave
     *  distance unmodified.
     */
    bool pick(const Eigen::ParametrizedLine<double, 3>& r, double& distance) const override;

    bool isNormalized() const override;

private:
    std::shared_ptr<const cmod::Model> m_model;
};
