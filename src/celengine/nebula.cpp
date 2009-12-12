// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
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
#include <cstdio>

using namespace Eigen;
using namespace std;


Nebula::Nebula() :
    geometry(InvalidResource)
{
}


const char* Nebula::getType() const
{
    return "Nebula";
}


void Nebula::setType(const string& /*typeStr*/)
{
}


size_t Nebula::getDescription(char* buf, size_t bufLength) const
{
    return snprintf(buf, bufLength, _("Nebula"));
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


bool Nebula::load(AssociativeArray* params, const string& resPath)
{
    string geometryFileName;
    if (params->getString("Mesh", geometryFileName))
    {
        ResourceHandle geometryHandle =
            GetGeometryManager()->getHandle(GeometryInfo(geometryFileName, resPath));
        setGeometry(geometryHandle);
    }

    return DeepSkyObject::load(params, resPath);
}


void Nebula::render(const GLContext& glcontext,
                    const Vector3f&,
                    const Quaternionf&,
                    float,
                    float pixelSize)
{
    Geometry* g = NULL;
    if (geometry != InvalidResource)
        g = GetGeometryManager()->find(geometry);
    if (g == NULL)
        return;

    glDisable(GL_BLEND);

    glScalef(getRadius(), getRadius(), getRadius());
    glRotate(getOrientation());

    if (glcontext.getRenderPath() == GLContext::GLPath_GLSL)
    {
        GLSLUnlit_RenderContext rc(getRadius());
        rc.setPointScale(2.0f * getRadius() / pixelSize);
        g->render(rc);
        glUseProgramObjectARB(0);
    }
    else
    {
        FixedFunctionRenderContext rc;
        rc.setLighting(false);
        g->render(rc);

        // Reset the material
        float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float zero = 0.0f;
        glColor4fv(black);
        glMaterialfv(GL_FRONT, GL_EMISSION, black);
        glMaterialfv(GL_FRONT, GL_SPECULAR, black);
        glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
    }

    glEnable(GL_BLEND);
}


unsigned int Nebula::getRenderMask() const
{
    return Renderer::ShowNebulae;
}


unsigned int Nebula::getLabelMask() const
{
    return Renderer::NebulaLabels;
}
