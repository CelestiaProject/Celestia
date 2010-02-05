// material.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "material.h"

using namespace cmod;

Material::Material() :
    diffuse(0.0f, 0.0f, 0.0f),
    emissive(0.0f, 0.0f, 0.0f),
    specular(0.0f, 0.0f, 0.0f),
    specularPower(1.0f),
    opacity(1.0f),
    blend(NormalBlend)
{
    for (int i = 0; i < TextureSemanticMax; ++i)
    {
        maps[i] = 0;
    }
}


Material::~Material()
{
    for (int i = 0; i < TextureSemanticMax; ++i)
    {
        delete maps[i];
    }
}
