// opencluster.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include "render.h"
#include "astro.h"
#include "opencluster.h"
#include "meshmanager.h"
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include <algorithm>

using namespace Eigen;
using namespace std;
using namespace celmath;


const char* OpenCluster::getType() const
{
    return "Open cluster";
}


void OpenCluster::setType(const std::string& /*typeStr*/)
{
}


string OpenCluster::getDescription() const
{
    return _("Open cluster");
}



const char* OpenCluster::getObjTypeName() const
{
    return "opencluster";
}


bool OpenCluster::pick(const Ray3d& ray,
                       double& distanceToPicker,
                       double& cosAngleToBoundCenter) const
{
    // The preconditional sphere-ray intersection test is enough for now:
    return DeepSkyObject::pick(ray, distanceToPicker, cosAngleToBoundCenter);
}


bool OpenCluster::load(AssociativeArray* params, const fs::path& resPath)
{
    // No parameters specific to open cluster, though a list of member stars
    // could be useful.
    return DeepSkyObject::load(params, resPath);
}


void OpenCluster::render(const Vector3f& /*unused*/,
                         const Quaternionf& /*unused*/,
                         float /*unused*/,
                         float /*unused*/,
                         const Renderer* /*unused*/) const
{
    // Nothing to do right now; open clusters are only visible as their
    // constituent stars and a label when labels are turned on.  A good idea
    // would be to add an 'sky chart' mode, in which clusters are rendered as
    // circles.
}


uint64_t OpenCluster::getRenderMask() const
{
    return Renderer::ShowOpenClusters;
}


unsigned int OpenCluster::getLabelMask() const
{
    return Renderer::OpenClusterLabels;
}
