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

#include <filesystem>
#include <iosfwd>
#include <memory>

#include <celutil/texhandle.h>
#include "model.h"

namespace cmod
{

class ModelLoader
{
public:
    std::unique_ptr<Model> load(std::istream& in);

protected:
    ~ModelLoader() = default;
    virtual celestia::util::TextureHandle getHandle(const std::filesystem::path&) = 0;

private:
    class BinaryLoader;
    class TextLoader;
};

class ModelWriter
{
public:
    bool saveBinary(const Model& model, std::ostream& out) const;
    bool saveText(const Model& model, std::ostream& out) const;

protected:
    ~ModelWriter() = default;
    virtual const std::filesystem::path* getPath(celestia::util::TextureHandle) const = 0;

private:
    class BinaryWriter;
    class TextWriter;
};

} // end namespace cmod
