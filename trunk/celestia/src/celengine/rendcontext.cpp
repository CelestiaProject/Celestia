// rendcontext.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rendcontext.h"
#include "texmanager.h"
#include "gl.h"
#include "glext.h"
#include "vecgl.h"


static Mesh::Material defaultMaterial;


RenderContext::RenderContext() :
    material(&defaultMaterial)
{
}


RenderContext::RenderContext(const Mesh::Material* _material)
{
    if (_material == NULL)
        material = &defaultMaterial;
    else
        material = _material;
}


void
RenderContext::makeCurrent()
{
    if (material->opacity == 1.0f)
    {
        blendOn = false;
        glDisable(GL_BLEND);
    }
    else
    {
        blendOn = true;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glColor4f(material->diffuse.red(),
              material->diffuse.green(),
              material->diffuse.blue(),
              material->opacity);

    if (material->specular == Color::Black)
    {
        float matSpecular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float zero = 0.0f;
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
        specularOn = false;
    }
    else
    {
        float matSpecular[4] = { material->specular.red(),
                                 material->specular.green(),
                                 material->specular.blue(),
                                 1.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, &material->specularPower);
        specularOn = true;
    }

    {
        float matEmissive[4] = { material->emissive.red(),
                                 material->emissive.green(),
                                 material->emissive.blue(),
                                 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, matEmissive);
    }


    Texture* t = NULL;
    if (material->tex0 != InvalidResource)
        t = GetTextureManager()->find(material->tex0);
    if (t == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        t->bind();
    }
}


void
RenderContext::setMaterial(const Mesh::Material* newMaterial)
{
    if (newMaterial == NULL)
        newMaterial = &defaultMaterial;

    if (newMaterial != material)
    {
        material = newMaterial;
        makeCurrent();
    }
}
