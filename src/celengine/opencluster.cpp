// opencluster.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
#include "render.h"
#include "astro.h"
#include "opencluster.h"
#include "meshmanager.h"
#include "vecgl.h"
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include <algorithm>
#include <cstdio>

using namespace Eigen;
using namespace std;


OpenCluster::OpenCluster()
{
}


const char* OpenCluster::getType() const
{
    return "Open cluster";
}


void OpenCluster::setType(const std::string& /*typeStr*/)
{
}


size_t OpenCluster::getDescription(char* buf, size_t bufLength) const
{
    return snprintf(buf, bufLength, _("Open cluster"));
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


bool OpenCluster::load(AssociativeArray* params, const string& resPath)
{
    // No parameters specific to open cluster, though a list of member stars
    // could be useful.
    return DeepSkyObject::load(params, resPath);
}


void OpenCluster::render(const GLContext&,
                         const Vector3f&,
                         const Quaternionf&,
                         float,
                         float)
{
    // Nothing to do right now; open clusters are only visible as their
    // constituent stars and a label when labels are turned on.  A good idea
    // would be to add an 'sky chart' mode, in which clusters are rendered as
    // circles.
}


unsigned int OpenCluster::getRenderMask() const
{
    return Renderer::ShowOpenClusters;
}


unsigned int OpenCluster::getLabelMask() const
{
    return Renderer::OpenClusterLabels;
}
