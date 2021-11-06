// modelfile.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <string>

#include "material.h"

namespace cmod
{
class Model;

/** Texture loading interface. Applications which want custom behavor for
  * texture loading should pass an instance of a TextureLoader subclass to
  * one of the model loading functions.
  */
class TextureLoader
{
public:
    virtual ~TextureLoader() {};
    virtual Material::TextureResource* loadTexture(const std::string& name) = 0;
};


Model* LoadModel(std::istream& in, TextureLoader* textureLoader = nullptr);

bool SaveModelAscii(const Model* model, std::ostream& out);
bool SaveModelBinary(const Model* model, std::ostream& out);

}
