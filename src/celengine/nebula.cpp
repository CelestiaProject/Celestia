// nebula.cpp
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
#include "nebula.h"
#include "meshmanager.h"
#include "rendcontext.h"
#include "gl.h"
#include "vecgl.h"

using namespace std;


Nebula::Nebula() :
    model(InvalidResource)
{
}


ResourceHandle Nebula::getModel() const
{
    return model;
}

void Nebula::setModel(ResourceHandle _model)
{
    model = _model;
}


bool Nebula::load(AssociativeArray* params, const string& resPath)
{
    string model;
    if (params->getString("Mesh", model))
    {
        ResourceHandle modelHandle =
            GetModelManager()->getHandle(ModelInfo(model, resPath));
        setModel(modelHandle);
    }
    
    return DeepSkyObject::load(params, resPath);
}


void Nebula::render(const Vec3f& offset,
                    const Quatf& viewerOrientation,
                    float brightness,
                    float pixelSize)
{
    Model* m = NULL;
    if (model != InvalidResource)
        m = GetModelManager()->find(model);
    if (m == NULL)
        return;

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glScalef(getRadius(), getRadius(), getRadius());
    glRotate(getOrientation());

    RenderContext rc;
    m->render(rc);

    glEnable(GL_BLEND);
}
