// dsodbbuilder.h
//
// Copyright (C) 2005-2024, the Celestia Development Team
//
// Split from dsodb.h - original version:
// Author: Toti <root@totibox>, (C) 2005
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

#include <celcompat/filesystem.h>
#include "astroobj.h"
#include "name.h"

class DeepSkyObject;
class DSODatabase;
class NameDatabase;

class DSODatabaseBuilder
{
public:
    DSODatabaseBuilder() = default;
    ~DSODatabaseBuilder();

    bool load(std::istream&, const fs::path& resourcePath = fs::path());
    std::unique_ptr<DSODatabase> finish();

private:
    std::vector<std::unique_ptr<DeepSkyObject>> DSOs;
    std::unique_ptr<NameDatabase> namesDB{ std::make_unique<NameDatabase>() };
    AstroCatalog::IndexNumber nextAutoCatalogNumber{ 0 };
};
