// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celmath/mathlib.h>
#include <celmath/vecgl.h>
#include <celutil/gettext.h>
#include "astro.h"
#include "meshmanager.h"
#include "nebula.h"
#include "rendcontext.h"
#include "render.h"


const char* Nebula::getType() const
{
    return "Nebula";
}


void Nebula::setType(const std::string& /*typeStr*/)
{
}


std::string Nebula::getDescription() const
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

DeepSkyObjectType Nebula::getObjType() const
{
    return DeepSkyObjectType::Nebula;
}


bool Nebula::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    // The preconditional sphere-ray intersection test is enough for now:
    return DeepSkyObject::pick(ray, distanceToPicker, cosAngleToBoundCenter);
}


bool Nebula::load(const AssociativeArray* params, const fs::path& resPath)
{
    if (const std::string* t = params->getString("Mesh"); t != nullptr)
    {
        fs::path geometryFileName(*t);
        ResourceHandle geometryHandle =
            GetGeometryManager()->getHandle(GeometryInfo(geometryFileName, resPath));
        setGeometry(geometryHandle);
    }

    return DeepSkyObject::load(params, resPath);
}


std::uint64_t Nebula::getRenderMask() const
{
    return Renderer::ShowNebulae;
}


unsigned int Nebula::getLabelMask() const
{
    return Renderer::NebulaLabels;
}
