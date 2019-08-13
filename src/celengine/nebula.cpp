// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include "vecgl.h"
#include "render.h"
#include "astro.h"
#include "nebula.h"
#include "meshmanager.h"
#include "rendcontext.h"
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include <algorithm>

using namespace Eigen;
using namespace std;
using namespace celmath;


const char* Nebula::getType() const
{
    return "Nebula";
}


void Nebula::setType(const string& /*typeStr*/)
{
}


string Nebula::getDescription() const
{
    return _("Nebula");
}


ResourceHandle Nebula::getGeometry() const
{
    return geometry;
}


void Nebula::setGeometry(ResourceHandle _geometry)
{
    geometry = _geometry;
}

const char* Nebula::getObjTypeName() const
{
    return "nebula";
}


bool Nebula::pick(const Ray3d& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    // The preconditional sphere-ray intersection test is enough for now:
    return DeepSkyObject::pick(ray, distanceToPicker, cosAngleToBoundCenter);
}


bool Nebula::load(AssociativeArray* params, const fs::path& resPath)
{
    string t;
    if (params->getString("Mesh", t))
    {
        fs::path geometryFileName(t);
        ResourceHandle geometryHandle =
            GetGeometryManager()->getHandle(GeometryInfo(geometryFileName, resPath));
        setGeometry(geometryHandle);
    }

    return DeepSkyObject::load(params, resPath);
}


void Nebula::render(const Vector3f& /*unused*/,
                    const Quaternionf& /*unused*/,
                    float /*unused*/,
                    float pixelSize,
                    const Renderer* renderer)
{
    Geometry* g = nullptr;
    if (geometry != InvalidResource)
        g = GetGeometryManager()->find(geometry);
    if (g == nullptr)
        return;

    glDisable(GL_BLEND);

    glScalef(getRadius(), getRadius(), getRadius());
    glRotate(getOrientation());

    GLSLUnlit_RenderContext rc(renderer, getRadius());
    rc.setPointScale(2.0f * getRadius() / pixelSize);
    g->render(rc);
    glUseProgram(0);

    glEnable(GL_BLEND);
}


uint64_t Nebula::getRenderMask() const
{
    return Renderer::ShowNebulae;
}


unsigned int Nebula::getLabelMask() const
{
    return Renderer::NebulaLabels;
}
