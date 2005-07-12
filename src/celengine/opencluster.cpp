// opencluster.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <stdio.h>
#include "celestia.h"
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include "astro.h"
#include "opencluster.h"
#include "meshmanager.h"
#include "gl.h"
#include "vecgl.h"
#include "render.h"

using namespace std;


OpenCluster::OpenCluster()
{
}


bool OpenCluster::load(AssociativeArray* params, const string& resPath)
{
    // No parameters specific to open cluster, though a list of member stars
    // could be useful.
    return DeepSkyObject::load(params, resPath);
}


void OpenCluster::render(const GLContext&,
                         const Vec3f& offset,
                         const Quatf& viewerOrientation,
                         float brightness,
                         float pixelSize)
{
    // Nothing to do right now; open clusters are only visible as their
    // constituent stars and a label when labels are turned on.  A good idea
    // would be to add an 'sky chart' mode, in which clusters are rendered as
    // circles.  
}


unsigned int OpenCluster::getRenderMask()
{
    return Renderer::ShowOpenClusters;
}


unsigned int OpenCluster::getLabelMask()
{
    return Renderer::OpenClusterLabels;
}
