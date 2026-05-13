// opencluster.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "opencluster.h"

#include <celutil/gettext.h>

const char*
OpenCluster::getType() const
{
    return "Open cluster";
}

void
OpenCluster::setType(const std::string& /*typeStr*/)
{
}

std::string
OpenCluster::getDescription() const
{
    return _("Open cluster");
}

DeepSkyObjectType
OpenCluster::getObjType() const
{
    return DeepSkyObjectType::OpenCluster;
}

bool
OpenCluster::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    // Open clusters are not rendered, do not pick them
    return false;
}

RenderFlags
OpenCluster::getRenderMask() const
{
    return RenderFlags::ShowOpenClusters;
}

RenderLabels
OpenCluster::getLabelMask() const
{
    return RenderLabels::OpenClusterLabels;
}
