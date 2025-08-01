// opencluster.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <Eigen/Geometry>

#include <celutil/reshandle.h>
#include "deepskyobj.h"
#include "renderflags.h"

class OpenCluster : public DeepSkyObject
{
public:
    OpenCluster() = default;

    const char* getType() const override;
    void setType(const std::string&) override;
    std::string getDescription() const override;

    bool pick(const Eigen::ParametrizedLine<double, 3>& ray,
              double& distanceToPicker,
              double& cosAngleToBoundCenter) const override;

    RenderFlags getRenderMask() const override;
    RenderLabels getLabelMask() const override;

    DeepSkyObjectType getObjType() const override;

    enum ClusterType
    {
        Open          = 0,
        Globular      = 1,
        NotDefined    = 2
    };
};
